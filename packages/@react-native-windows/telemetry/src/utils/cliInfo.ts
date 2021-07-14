/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import {randomBytes} from 'crypto';
import {execSync} from 'child_process';
import semver from 'semver';

export function isCliTest(): boolean {
  return process.env.RNW_CLI_TEST === 'true';
}

export function generateSessionId(): string {
  return randomBytes(16).toString('hex');
}

function getVersionFromCommand(command: string): string | null {
  const version = execSync(command)
    .toString()
    .trim();
  return semver.valid(version);
}

export function getNodeVersion(): string | null {
  return process.version;
}

export function getNpmVersion(): string | null {
  return getVersionFromCommand('npm -v');
}

export function getYarnVersion(): string | null {
  return getVersionFromCommand('yarn -v');
}

export function getVersions(): Record<string, string> {
  const vers: Record<string, string> = {};

  const nodeVersion = getNodeVersion();
  if (nodeVersion) {
    vers.node = nodeVersion;
  }

  const npmVersion = getNpmVersion();
  if (npmVersion) {
    vers.npm = npmVersion;
  }

  const yarnVersion = getYarnVersion();
  if (yarnVersion) {
    vers.yarn = yarnVersion;
  }

  return vers;
}
