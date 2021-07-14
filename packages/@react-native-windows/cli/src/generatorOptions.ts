/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import yargs from 'yargs';

/**
 * Generator-specific CLI options that will be injected into the yargs instance
 * used by react-native-windows-init
 */
const yargsOptions = {
  language: {
    type: 'string' as 'string',
    describe: 'The language the project is written in.',
    choices: ['cs', 'cpp'],
    default: 'cpp',
  },
  overwrite: {
    type: 'boolean' as 'boolean',
    describe: 'Overwrite any existing files without prompting',
    default: false,
  },
  projectType: {
    type: 'string' as 'string',
    describe: 'The type of project to initialize',
    choices: ['app', 'lib'],
    default: 'app',
  },
  experimentalNuGetDependency: {
    type: 'boolean' as 'boolean',
    describe:
      '[Experimental] change to start consuming a NuGet containing a pre-built dll version of Microsoft.ReactNative',
    hidden: true,
    default: false,
  },
  useHermes: {
    type: 'boolean' as 'boolean',
    describe: '[Experimental] Use Hermes instead of Chakra as the JS engine',
    default: false,
  },
  useWinUI3: {
    type: 'boolean' as 'boolean',
    describe: '[Experimental] Use WinUI3',
    hidden: true,
    default: false,
  },
  nuGetTestVersion: {
    type: 'string' as 'string',
    describe:
      '[internalTesting] By default the NuGet version matches the rnw package. This flag allows manually specifying the version for internal testing.',
    hidden: true,
  },
  nuGetTestFeed: {
    type: 'string' as 'string',
    describe:
      '[internalTesting] Allows a test feed to be added to the generated NuGet configuration',
    hidden: true,
  },
};

/**
 * Inject options into the existing yargs singleton. This should be called when
 * registering other yargs options, before project generation.
 */
export function registerCliOptions(yargv: yargs.Argv) {
  yargv.options(yargsOptions);
}

/**
 * Options which are provided via generator controlled CLI args
 */
export type GeneratorCliOptions = yargs.InferredOptionTypes<
  typeof yargsOptions
>;

/**
 * Options to be passed from react-native-windows-init
 */
export type GeneratorInitOptions = {
  verbose: boolean;
  useDevMode: boolean;
};

export type GeneratorOptions = GeneratorCliOptions & GeneratorInitOptions;
