/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as React from 'react';
import {Image} from 'react-native';
import {functionTest, componentTest} from './lib/TestDefinition';

/**
 * An example of a passing test
 */
functionTest('SampleFunctionTest', () => {
  if (2 + 2 !== 4) {
    throw new Error('Math is hard');
  }
});

/**
 * Test methods can also be async
 */
functionTest('SampleAsyncFunctionTest', async () => {
  await new Promise(resolve => setTimeout(resolve, 10));
});

/**
 * Tests can be written as React Components which call callbacks
 */
componentTest('SampleComponentTest', ({pass, fail}) => {
  return (
    <Image
      source={{
        uri:
          'https://github.com/microsoft/react-native-windows/blob/0.63-stable/packages/react-native-platform-override/src/e2etest/collateral/0.59.9/Icon-60@2x.conflict.png?raw=true',
      }}
      onLoad={() => pass()}
      onError={() => fail(new Error("Couldn't load image"))}
    />
  );
});

/**
 * Failing tests can be marked as skipped
 */
componentTest.skip('SampleSkippedTest', ({pass, fail}) => {
  return (
    <Image
      source={{uri: 'https://google.com'}}
      onLoad={() => pass()}
      onError={() => fail(new Error("Couldn't load image"))}
    />
  );
});
