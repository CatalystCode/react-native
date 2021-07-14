/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import {GenerateProject} from './GeneratorTypes';

import yargs from 'yargs';

/**
 * Options expected by project generators prior to 0.66
 */
type LegacyGeneratorOptions = {
  overwrite: boolean;
  language: string;
  projectType: string;
  experimentalNuGetDependency: boolean;
  nuGetTestVersion?: string;
  nuGetTestFeed?: string;
  useWinUI3: boolean;
  useHermes: boolean;
  useDevMode: boolean;
  verbose: boolean;
};

// 0.62 - 0.65: Project generation requires us to pass options
type GenerateProject65 = (
  projectDir: string,
  name: string,
  ns: string,
  options: LegacyGeneratorOptions,
) => Promise<void>;

// <= 0.62: Function is syncroonus and requires us to pass options
type GenerateProject62 = (
  projectDir: string,
  name: string,
  ns: string,
  options: LegacyGeneratorOptions,
) => void;

// Project generation flags to use for RNW versions prior to 0.66 (where this is provided in the CLI package)
const legacyCliOptions = {
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
    describe: 'The type of project to initialize (supported on 0.64+)',
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
    describe:
      '[Experimental] Use Hermes instead of Chakra as the JS engine (supported on 0.64+ for C++ projects)',
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

type LegacyCliOptions = yargs.InferredOptionTypes<typeof legacyCliOptions>;

export function registerCliOptions(yargv: yargs.Argv) {
  const registrationSucceeded = tryRequireChain([
    () => {
      // 0.66+
      const generator = require(require.resolve(
        'react-native-windows/generator',
        {
          paths: [process.cwd()],
        },
      ));
      generator.registerCliOptions(yargv);
      return true;
    },
  ]);

  if (!registrationSucceeded) {
    yargv.options(legacyCliOptions);
  }
}

/**
 * Returns the Windows template generator, normalizing to any differences
 * across RN versions.
 */
export function requireGenerateProject(): GenerateProject | null {
  return tryRequireChain<GenerateProject>([
    () => {
      const generator = require(require.resolve(
        'react-native-windows/generator',
        {
          paths: [process.cwd()],
        },
      ));
      return generator.generateProject;
    },
    () => {
      const generateProject65: GenerateProject65 = require(require.resolve(
        'react-native-windows/generate',
        {
          paths: [process.cwd()],
        },
      ));

      const argv = (yargs.argv as unknown) as LegacyCliOptions;
      return (projectDir, name, ns, initOptions) =>
        generateProject65(projectDir, name, ns, {...argv, ...initOptions});
    },
    () => {
      const generateProject62: GenerateProject62 = require(require.resolve(
        'react-native-windows/local-cli/generate-windows',
        {paths: [process.cwd()]},
      ));

      const argv = (yargs.argv as unknown) as LegacyCliOptions;
      return async (projectDir, name, ns, initOptions) =>
        generateProject62(projectDir, name, ns, {...argv, ...initOptions});
    },
  ]);
}

/**
 * Executes each attempt to require in order, returning the result of the
 * first function to not throw 'MODULE_NOT_FOUND'
 */
function tryRequireChain<T>(attempts: Array<() => T>): T | null {
  if (attempts.length === 0) {
    return null;
  }

  const [firstAttempt, ...rest] = attempts;
  try {
    return firstAttempt();
  } catch (ex) {
    if (ex.code === 'MODULE_NOT_FOUND') {
      return tryRequireChain<T>(rest);
    } else {
      return null;
    }
  }
}
