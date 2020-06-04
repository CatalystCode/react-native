/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import {hashContent} from '../Hash';

test('hashContent - Different Strings', () => {
  expect(hashContent('a')).not.toBe(hashContent('b'));
});

test('hashContent - Insensitive to Line Ending Type', () => {
  expect(hashContent('a\r\n')).toBe(hashContent('a\n'));
});

test('hashContent - Repeated Different Line Endings', () => {
  expect(hashContent('a\r\n\r\n')).toBe(hashContent('a\n\n'));
});

test('hashContent - Line Endings are Visible', () => {
  expect(hashContent('a')).not.toBe(hashContent('a\n\n'));
});
