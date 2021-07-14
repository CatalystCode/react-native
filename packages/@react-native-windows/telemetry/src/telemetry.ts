/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import path from 'path';
import * as appInsights from 'applicationinsights';
import {execSync} from 'child_process';
import {CodedError} from './CodedError';

import * as baseInfo from './utils/baseInfo';
import * as cliInfo from './utils/cliInfo';

export interface TelemetryOptions {
  setupString: string;
  shouldDisable: boolean;
  preserveErrorMessages: boolean;
}

export class Telemetry {
  static client?: appInsights.TelemetryClient | undefined = undefined;
  static options: TelemetryOptions = {
    setupString: '795006ca-cf54-40ee-8bc6-03deb91401c3',
    shouldDisable: false,
    preserveErrorMessages: false,
  };

  /** Sets up the Telemetry static to be used elsewhere. */
  static setup(options?: Partial<TelemetryOptions>) {
    if (Telemetry.client) {
      // Bail since we've already setup
      return;
    }

    // Save off options for later
    Object.assign(Telemetry.options, options);

    Telemetry.setupClient();

    Telemetry.setupBaseProperties();
    Telemetry.setupCliProperties();
    Telemetry.setupTelemetryProcessors();
  }

  /** Sets up Telemetry.client. */
  private static setupClient() {
    if (!cliInfo.isCliTest()) {
      appInsights.Configuration.setInternalLogging(false, false);
    }

    appInsights.setup(this.options.setupString);
    Telemetry.client = appInsights.defaultClient;

    // Disable
    if (
      Telemetry.options.shouldDisable ||
      (baseInfo.isCI() && !baseInfo.captureCI())
    ) {
      this.disable();
    }
  }

  /** Sets up any base properties that all telemetry events require. */
  private static setupBaseProperties() {
    Telemetry.client!.commonProperties.deviceId = baseInfo.deviceId();
    Telemetry.client!.commonProperties.deviceLocale = baseInfo.deviceLocale();
    Telemetry.client!.commonProperties.ciCaptured = baseInfo
      .captureCI()
      .toString();
    Telemetry.client!.commonProperties.ciType = baseInfo.ciType();
    Telemetry.client!.commonProperties.isMsftInternal = baseInfo
      .isMsftInternal()
      .toString();

    Telemetry.client!.config.samplingPercentage = baseInfo.sampleRate();
  }

  /** Sets up any properties that CLI telemetry events require. */
  private static setupCliProperties() {
    if (cliInfo.isCliTest()) {
      Telemetry.client!.commonProperties.isTest = 'true';
    }

    Telemetry.client!.commonProperties.sessionId = cliInfo.generateSessionId();
  }

  /** Sets up any telemetry processors  */
  private static setupTelemetryProcessors() {
    if (Telemetry.client) {
      Telemetry.client.addTelemetryProcessor(sanitizeEnvelope);
    }
  }

  static enable() {
    if (Telemetry.client) {
      Telemetry.client.config.disableAppInsights = false;
    }
    Telemetry.options.shouldDisable = false;
  }

  static disable() {
    if (Telemetry.client) {
      Telemetry.client.config.disableAppInsights = true;
    }
    Telemetry.options.shouldDisable = true;
  }

  static trackException(e: Error, properties?: Record<string, any>) {
    const props: Record<string, any> = {};
    if (e instanceof CodedError) {
      Object.assign(props, (e as CodedError).data);
    }

    const syscallExceptionFieldsToCopy = ['errno', 'syscall', 'code'];
    for (const f of syscallExceptionFieldsToCopy) {
      if ((e as any)[f]) {
        props[f] = [f];
      }
    }
    Object.assign(props, props, properties);
    Telemetry.client?.trackException({
      exception: e,
      properties: props,
    });
  }
}

function getAnonymizedPath(filepath: string): string {
  const projectRoot = process.cwd().toLowerCase();
  filepath = filepath.replace(/\//g, '\\');
  const knownPathsVars = ['AppData', 'LocalAppData', 'UserProfile'];
  if (filepath.toLowerCase().startsWith(projectRoot)) {
    const ext = path.extname(filepath);
    const rest = filepath.slice(projectRoot.length);
    const nodeModules = '\\node_modules\\';
    // this is in the project dir but not under node_modules
    if (rest.toLowerCase().startsWith('\\windows\\')) {
      return `[windows]\\???${ext}(${filepath.length})`;
    } else if (rest.toLowerCase().startsWith(nodeModules)) {
      return 'node_modules' + rest.slice(nodeModules.length - 1);
    } else {
      return `[project_dir]\\???${ext}(${filepath.length})`;
    }
  } else {
    for (const knownPath of knownPathsVars) {
      if (
        process.env[knownPath] &&
        filepath.toLowerCase().startsWith(process.env[knownPath]!.toLowerCase())
      ) {
        return `[${knownPath}]\\???(${filepath.length})`;
      }
    }
  }
  return '[path]';
}
/**
 * Sanitize any paths that appear between quotes (''), brackets ([]), or double quotes ("").
 * @param msg the string to sanitize
 */
export function sanitizeMessage(msg: string): string {
  const cpuThreadId = /^\d+(:\d+)?>/g;
  msg = msg.replace(cpuThreadId, '');
  const parts = msg.split(/['[\]"]/g);
  const clean = [];
  const pathRegEx = /([A-Za-z]:|\\)[\\/]([^<>:;,?"*\t\r\n|/\\]+[\\/])+([^<>:;,?"*\t\r\n|]+\/?)/gi;
  for (const part of parts) {
    if (pathRegEx.test(part)) {
      pathRegEx.lastIndex = -1;
      let matches: RegExpExecArray | null;
      let noPath = '';
      let last = 0;
      while ((matches = pathRegEx.exec(part))) {
        noPath +=
          part.substr(last, matches!.index - last) +
          getAnonymizedPath(matches[0]);
        last = matches!.index + matches![0].length;
      }
      clean.push(noPath);
    } else if (part !== '') {
      clean.push(part);
    }
  }
  return clean.join(' ').trim();
}

export function sanitizeFrame(frame: any): void {
  const parens = frame.method.indexOf('(');
  if (parens !== -1) {
    // case 1: method === 'methodName (rootOfThePath'
    frame.method = frame.method.substr(0, parens).trim();
  } else {
    // case 2: method === <no_method> or something without '(', fileName is full path
  }
  // preserve only the last_directory/filename
  frame.fileName = path.join(
    path.basename(path.dirname(frame.fileName)),
    path.basename(frame.fileName),
  );
  frame.assembly = '';
}

export function tryGetErrorCode(msg: string): string | undefined {
  const errorRegEx = /error (\w+\d+):/gi;
  const m = errorRegEx.exec(msg);
  return m ? m[1] : undefined;
}

/**
 * Remove PII from exceptions' stack traces and messages
 * @param envelope the telemetry envelope. Provided by AppInsights.
 */
export function sanitizeEnvelope(envelope: any /*context: any*/): boolean {
  if (envelope.data.baseType === 'ExceptionData') {
    const data = envelope.data.baseData;
    for (const exception of data.exceptions || []) {
      for (const frame of exception.parsedStack) {
        sanitizeFrame(frame);
      }
      const errorCode = tryGetErrorCode(exception.message);
      data.properties.errorCode = errorCode;
      // CodedError has non-PII information in its 'type' member, plus optionally some more info in its 'data'.
      // The message may contain PII information. This can be sanitized, but for now delete it.
      // Note that the type of data.exceptions[0] is always going to be ExceptionDetails. It is not the original thrown exception.
      // https://github.com/microsoft/ApplicationInsights-node.js/issues/707
      if (Telemetry.options.preserveErrorMessages) {
        exception.message = sanitizeMessage(exception.message);
      } else {
        delete exception.message;
      }
    }
  }
  delete envelope.tags['ai.cloud.roleInstance'];
  return true;
}

export function isMSFTInternal(): boolean {
  return (
    process.env.UserDNSDomain !== undefined &&
    process.env.UserDNSDomain.toLowerCase().endsWith('.microsoft.com')
  );
}

export function getDiskFreeSpace(drivePath: string | null): number {
  const out = execSync(`dir /-C ${drivePath}`)
    .toString()
    .split('\r\n');
  const line = out[out.length - 2];
  const result = line.match(/(\d+) [^\d]+(\d+) /);
  if (result && result.length > 2) {
    return Number(result[2]);
  }
  return -1;
}
