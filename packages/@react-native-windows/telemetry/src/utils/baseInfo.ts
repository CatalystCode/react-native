/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import ci from 'ci-info';
import {machineIdSync} from 'node-machine-id';
import osLocale from 'os-locale';

export function deviceId(): string {
  return machineIdSync(false);
}

export function deviceLocale(): string {
  return osLocale.sync();
}

export function sampleRate(): number {
  return 100;
}

export function captureCI(): boolean {
  return false;
}

export function isCI(): boolean {
  return ci.isCI;
}

export function ciType(): string {
  return ci.isCI ? ci.name ?? 'Unknown' : 'None';
}

export function isMsftInternal(): boolean {
  return (
    process.env.UserDNSDomain !== undefined &&
    process.env.UserDNSDomain.toLowerCase().endsWith('corp.microsoft.com')
  );
}
