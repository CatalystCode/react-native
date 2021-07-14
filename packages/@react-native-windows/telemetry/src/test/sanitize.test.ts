/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import {
  Telemetry,
  sanitizeMessage,
  sanitizeEnvelope,
  sanitizeFrame,
  tryGetErrorCode,
} from '../telemetry';
import * as appInsights from 'applicationinsights';
import {basename} from 'path';
import {CodedError} from '../CodedError';

delete process.env.AGENT_NAME; // allow this test to run in Azure DevOps / GHA
Telemetry.setup({preserveErrorMessages: true, shouldDisable: true});

test('Sanitize message, no-op', () => {
  // do stuff
  expect(sanitizeMessage('')).toEqual('');
  expect(sanitizeMessage('some text')).toEqual('some text');
});

test('Sanitize message, project_dir', () => {
  expect(sanitizeMessage(`this is the cwd: '${process.cwd()}'`)).toEqual(
    `this is the cwd:  [project_dir]\\???(${process.cwd().length})`,
  );
  expect(
    sanitizeMessage(`uppercase: '${process.cwd().toUpperCase()}'`),
  ).toEqual(`uppercase:  [project_dir]\\???(${process.cwd().length})`);
  expect(
    sanitizeMessage(`lowercase: '${process.cwd().toLowerCase()}'`),
  ).toEqual(`lowercase:  [project_dir]\\???(${process.cwd().length})`);
  expect(
    sanitizeMessage(`this is the cwd: '${process.cwd()}' and something else`),
  ).toEqual(
    `this is the cwd:  [project_dir]\\???(${
      process.cwd().length
    })  and something else`,
  );
  expect(
    sanitizeMessage(`this is the cwd: ${process.cwd()} and something else`),
  ).toEqual(
    `this is the cwd: [project_dir]\\???(${
      (process.cwd() + ' and something else').length
    })`,
  );
});

test('Sanitize message, node_modules', () => {
  expect(
    sanitizeMessage(`this is the cwd: '${process.cwd()}\\node_modules'`),
  ).toEqual(
    `this is the cwd:  [project_dir]\\???(${
      (process.cwd() + '\\node_modules').length
    })`,
  );
  expect(
    sanitizeMessage(`this is the cwd: '${process.cwd()}\\node_modules\\foo'`),
  ).toEqual('this is the cwd:  node_modules\\foo');
  expect(
    sanitizeMessage(
      `uppercase: '${process.cwd().toUpperCase()}\\NODE_MODULES\\foo'`,
    ),
  ).toEqual('uppercase:  node_modules\\foo');
  expect(
    sanitizeMessage(
      `lowercase: '${process.cwd().toLowerCase()}\\NODE_MODULES\\'`,
    ),
  ).toEqual('lowercase:  node_modules\\');
  expect(
    sanitizeMessage(
      `trailing: '${process.cwd()}\\node_modules\\' and something else`,
    ),
  ).toEqual('trailing:  node_modules\\  and something else');
  expect(
    sanitizeMessage(
      `this is the cwd: ${process.cwd()}\\node_modules and something else that could be part of the path`,
    ),
  ).toEqual(
    `this is the cwd: [project_dir]\\???(${
      (
        process.cwd() +
        '\\node_modules and something else that could be part of the path'
      ).length
    })`,
  );
  expect(
    sanitizeMessage(
      `this is the cwd: ${process.cwd()}\\node_modules\\ a file under nm`,
    ),
  ).toEqual(`this is the cwd: node_modules\\ a file under nm`);
});

test('Sanitize message, other path', () => {
  expect(sanitizeMessage(`this is another path: 'A:\\foo\\bar\\baz'`)).toEqual(
    `this is another path:  [path]`,
  );

  expect(sanitizeMessage(`this is another path: A:\\foo\\bar\\baz`)).toEqual(
    `this is another path: [path]`,
  );

  expect(
    sanitizeMessage(
      `Cannot find module 'react-native/package.json'
      Require stack:
      - ${process.env.AppData}\\npm-cache\\_npx\\1384\\node_modules\\react-native-windows-init\\lib-commonjs\\Cli.js
      - ${process.env.AppData}\\npm-cache\\_npx\\1384\\node_modules\\react-native-windows-init\\bin.js`,
    ),
  ).toEqual(`Cannot find module  react-native/package.json 
      Require stack:
      - [AppData]\\???(${
        (
          process.env.AppData +
          '\\npm-cache\\_npx\\1384\\node_modules\\react-native-windows-init\\lib-commonjs\\Cli.js'
        ).length
      })
      - [AppData]\\???(${
        (
          process.env.AppData +
          '\\npm-cache\\_npx\\1384\\node_modules\\react-native-windows-init\\bin.js'
        ).length
      })`);
});

test('Sanitize message, forward slashes', () => {
  expect(
    sanitizeMessage(
      `EPERM: operation not permitted, scandir  ${process.env.UserProfile!.replace(
        /\\/g,
        '/',
      )}/source/repos/rn2/wintest/windows/packages/boost.1.76.0.0/lib/native/include`,
    ),
  ).toEqual(
    `EPERM: operation not permitted, scandir  [UserProfile]\\???(${
      (
        process.env.UserProfile +
        '/source/repos/rn2/wintest/windows/packages/boost.1.76.0.0/lib/native/include'
      ).length
    })`,
  );
});

test('Sanitize message, file share path', () => {
  expect(sanitizeMessage(`\\\\server\\share`)).toEqual('[path]');
});

test('Sanitize message, with cpu/thread id', () => {
  expect(sanitizeMessage('5>This is an error')).toEqual('This is an error');

  expect(sanitizeMessage('5:42>This is an error')).toEqual('This is an error');
});

test('Error code', () => {
  expect(tryGetErrorCode('foo bar error FOO2020: the thing')).toEqual(
    'FOO2020',
  );
});

test('Sanitize stack frame', () => {
  const emptyFrame: appInsights.Contracts.StackFrame = {
    level: 0,
    method: '',
    fileName: '',
    assembly: 'asdf',
    line: 0,
  };
  sanitizeFrame(emptyFrame);
  expect(emptyFrame).toEqual({
    level: 0,
    assembly: '',
    fileName: '.',
    method: '',
    line: 0,
  });

  const frame1: appInsights.Contracts.StackFrame = {
    method: '',
    fileName: `${process.cwd()}\\foo.js`,
    assembly: 'asdf',
    level: 0,
    line: 0,
  };
  sanitizeFrame(frame1);
  expect(frame1).toEqual({
    assembly: '',
    fileName: 'telemetry\\foo.js',
    method: '',
    level: 0,
    line: 0,
  });

  const frame2: appInsights.Contracts.StackFrame = {
    method: `myMethod (something ${process.cwd()}`,
    fileName: `${process.cwd()}\\foo.js`,
    assembly: 'asdf',
    level: 1,
    line: 42,
  };
  sanitizeFrame(frame2);
  expect(frame2).toEqual({
    assembly: '',
    fileName: 'telemetry\\foo.js',
    method: 'myMethod',
    level: 1,
    line: 42,
  });
});

test('basic setup', () => {
  expect(Telemetry.client!.commonProperties.sessionId).toBeDefined();

  expect(
    Telemetry.client!.commonProperties.sessionId.length,
  ).toBeGreaterThanOrEqual(32);

  expect(Telemetry.client!.commonProperties.isTest).toEqual('true');
});

function b(s: string) {
  throw new Error('hello ' + s);
}
function a(s: string) {
  b(s);
}

// #7903: This test assumes exact structure of stack trace, which can be flaky or implementation dependent
test.skip('thrown exception a->b, hello world', () => {
  let pass = false;
  Telemetry.client!.addTelemetryProcessor((envelope, _) => {
    if (envelope.data.baseType === 'ExceptionData') {
      const data = (envelope.data as any).baseData;
      expect(data.exceptions).toBeDefined();
      expect(data.exceptions.length).toEqual(1);
      expect(data.exceptions[0].message).toEqual('hello world');

      const stack = data.exceptions[0].parsedStack;
      expect(stack).toBeDefined();
      expect(stack.length).toBeGreaterThan(5);

      const filename = basename(__filename);
      expect(stack[0].method).toEqual('b');
      expect(stack[1].method).toEqual('b');
      expect(stack[2].method).toEqual('Object.a');
      expect(stack[0].fileName).toEqual(`test\\${filename}`);
      expect(stack[1].fileName).toEqual(`test\\${filename}`);
      expect(stack[2].fileName).toEqual(`test\\${filename}`);
      pass = true;
    }
    return true;
  });
  try {
    a('world');
  } catch (e) {
    Telemetry.client!.trackException({exception: e});
  }
  Telemetry.client!.flush();

  expect(pass).toBeTruthy();
  Telemetry.client!.clearTelemetryProcessors();
  Telemetry.client!.addTelemetryProcessor(sanitizeEnvelope);
});

test('throw exception with error code', done => {
  Telemetry.client!.addTelemetryProcessor((envelope, _) => {
    const data = (envelope.data as any).baseData;
    expect(data.properties.errorCode).toEqual('FOO2020');
    return true;
  });

  try {
    throw new Error('hello from an error FOO2020: the error string');
  } catch (e) {
    Telemetry.client!.trackException({exception: e});
    Telemetry.client!.flush();
  }
  Telemetry.client!.clearTelemetryProcessors();
  Telemetry.client!.addTelemetryProcessor(sanitizeEnvelope);
  done();
});

// #7903: This test assumes exact structure of stack trace, which can be flaky or implementation dependent
test.skip('thrown exception a->b, hello path', done => {
  let pass = false;
  Telemetry.client!.addTelemetryProcessor((envelope, _) => {
    if (envelope.data.baseType === 'ExceptionData') {
      const data = (envelope.data as any).baseData;
      expect(data.exceptions).toBeDefined();
      expect(data.exceptions.length).toEqual(1);
      expect(data.exceptions[0].message).toEqual(
        `hello [project_dir]\\???(${process.cwd().length})`,
      );

      const stack = data.exceptions[0].parsedStack;
      expect(stack).toBeDefined();
      expect(stack.length).toBeGreaterThan(5);

      const filename = basename(__filename);
      expect(stack[0].method).toEqual('b');
      expect(stack[1].method).toEqual('b');
      expect(stack[2].method).toEqual('Object.a');
      expect(stack[0].fileName).toEqual(`test\\${filename}`);
      expect(stack[1].fileName).toEqual(`test\\${filename}`);
      expect(stack[2].fileName).toEqual(`test\\${filename}`);
      pass = true;
    }
    return true;
  });
  try {
    a(process.cwd());
  } catch (e) {
    Telemetry.client!.trackException({exception: e});
  }
  Telemetry.client!.flush();

  expect(pass).toBeTruthy();
  expect(pass).toBeTruthy();
  Telemetry.client!.clearTelemetryProcessors();
  Telemetry.client!.addTelemetryProcessor(sanitizeEnvelope);
  done();
});

test('trackEvent should not identify roleInstance', () => {
  Telemetry.client!.addTelemetryProcessor((envelope, _) => {
    expect(envelope.tags['ai.cloud.roleInstance']).toBeUndefined();
    return true;
  });
  Telemetry.client!.trackEvent({
    name: 'test',
    properties: {},
  });
  Telemetry.client!.flush();
  Telemetry.client!.clearTelemetryProcessors();
  Telemetry.client!.addTelemetryProcessor(sanitizeEnvelope);
});

/////////////////////////
// CodedError tests
test('No message', done => {
  Telemetry.options.preserveErrorMessages = false;
  let pass = false;
  Telemetry.client!.addTelemetryProcessor((envelope, _) => {
    if (envelope.data.baseType === 'ExceptionData') {
      const data = (envelope.data as any).baseData;
      expect(data.exceptions).toBeDefined();
      expect(data.exceptions.length).toEqual(1);
      expect(data.exceptions[0].message).toBeUndefined();
      expect(data.exceptions[0].typeName).toEqual('MSBuildError');
      // This should be 42, but instead it is '42'
      // https://github.com/microsoft/ApplicationInsights-node.js/issues/708
      expect(data.properties.foo).toBeDefined();
      expect(parseInt(data.properties.foo, 10)).toEqual(42);
      pass = true;
    }
    return true;
  });

  try {
    throw new CodedError('MSBuildError', 'Something secret', {
      foo: 42,
    });
  } catch (e) {
    Telemetry.client!.trackException({
      exception: e,
      properties: (e as CodedError).data,
    });
    Telemetry.client!.flush();
  }

  expect(pass).toEqual(true);
  Telemetry.client!.clearTelemetryProcessors();
  Telemetry.client!.addTelemetryProcessor(sanitizeEnvelope);
  done();
});
