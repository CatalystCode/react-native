/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as baseInfo from '../utils/baseInfo';

test('deviceId() is valid', () => {
  const value = baseInfo.deviceId();
  expect(value).toBeDefined();
  expect(value).not.toBe('');
  expect(value).not.toBeNull();
});

test('deviceId() does not change', () => {
  expect(baseInfo.deviceId()).toEqual(baseInfo.deviceId());
});

test('deviceLocale() is valid', () => {
  const value = baseInfo.deviceLocale();
  expect(value).toBeDefined();
  expect(value).not.toBe('');
  expect(value).not.toBeNull();
});

test('deviceLocale() does not change', () => {
  expect(baseInfo.deviceLocale()).toEqual(baseInfo.deviceLocale());
});

test('sampleRate() is within valid range', () => {
  const value = baseInfo.sampleRate();
  expect(value).toBeGreaterThanOrEqual(0);
  expect(value).toBeLessThanOrEqual(100);
});

test('ciType() is valid', () => {
  const value = baseInfo.ciType();
  expect(value).toBeDefined();
  expect(value).not.toBe('');
  expect(value).not.toBeNull();
});

test('ciType() is None when not in CI', () => {
  if (baseInfo.isCI()) {
    expect(baseInfo.ciType()).not.toBe('None');
  } else {
    expect(baseInfo.ciType()).toBe('None');
  }
});

test('isMsftInternal() is false with no domain', () => {
  delete process.env.UserDNSDomain;
  expect(baseInfo.isMsftInternal()).toBe(false);
});

test('isMsftInternal() is false with example.com domain', () => {
  process.env.UserDNSDomain = 'example.com';
  expect(baseInfo.isMsftInternal()).toBe(false);
});

test('isMsftInternal() is true with Msft domain', () => {
  process.env.UserDNSDomain = 'test.corp.microsoft.com';
  expect(baseInfo.isMsftInternal()).toBe(true);
});
