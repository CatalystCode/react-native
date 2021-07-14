/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 * @ts-check
 */

// For a detailed explanation regarding each configuration property, visit:
// https://jestjs.io/docs/en/configuration.html

module.exports = {
  // A list of paths to directories that Jest should use to search for files in
  roots: ['<rootDir>/test/'],

  // The test environment that will be used for testing
  testEnvironment: 'jest-environment-winappdriver',

  // The pattern or patterns Jest uses to detect test files
  testRegex: '.*\\.test\\.ts$',

  // Default timeout of a test in milliseconds
  testTimeout: 70000,

  // A map from regular expressions to paths to transformers
  transform: {
    '^.+\\.[jt]sx?$': ['babel-jest', require('@rnw-scripts/babel-node-config')],
  },

  // Specifies the maximum number of workers the worker-pool will spawn for running tests.
  maxWorkers: 1,

  // Indicates whether each individual test should be reported during the run.
  verbose: true,

  // A list of paths to modules that run some code to configure or set up the testing framework
  // before each test file in the suite is executed
  setupFilesAfterEnv: ['./jest.setup.js'],

  testEnvironmentOptions: {
    app: 'ReactUWPTestApp_8wekyb3d8bbwe!App',
    enableRpc: true,
    webdriverOptions: {
      // Level of logging verbosity: trace | debug | info | warn | error
      logLevel: 'error',

      // Default timeout for all waitFor* commands.
      waitforTimeout: 60000,

      // Default timeout in milliseconds for request
      connectionRetryTimeout: 90000,

      // Default request retries count
      connectionRetryCount: 10,
    },
  },
};
