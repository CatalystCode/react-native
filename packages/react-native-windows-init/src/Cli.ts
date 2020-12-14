/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as yargs from 'yargs';
import * as fs from 'fs';
import * as semver from 'semver';
import {exec, execSync} from 'child_process';
import * as validUrl from 'valid-url';
import * as prompts from 'prompts';
import * as findUp from 'find-up';
import * as chalk from 'chalk';
// @ts-ignore
import * as Registry from 'npm-registry';

import {Telemetry, isMSFTInternal} from '@react-native-windows/telemetry';

/**
 * Important:
 * Do not use process.exit() in this script as it will prevent telemetry from being sent.
 * See https://github.com/microsoft/ApplicationInsights-node.js/issues/580
 */

import requireGenerateWindows from './requireGenerateWindows';

const npmConfReg = execSync('npm config get registry')
  .toString()
  .trim();
const NPM_REGISTRY_URL = validUrl.isUri(npmConfReg)
  ? npmConfReg
  : 'http://registry.npmjs.org';
const npm = new Registry({registry: NPM_REGISTRY_URL});

const exitCodes = {
  SUCCESS: 0,
  UNSUPPORTED_VERSION_RN: 3,
  USER_CANCEL: 4,
  NO_REACTNATIVE_FOUND: 5,
  UNKNOWN_ERROR: 6,
  NO_PACKAGE_JSON: 7,
  NO_LATEST_RNW: 8,
  NO_AUTO_MATCHING_RNW: 9,
  INCOMPATIBLE_OPTIONS: 10,
  NO_REACTNATIVE_DEPENDENCIES: 12,
};

type UserExitCode = keyof typeof exitCodes;

class UserError extends Error {
  exitCode: UserExitCode;

  constructor(exitCode: UserExitCode, message?: string) {
    super(message);
    this.exitCode = exitCode;
  }
}

const argv = yargs
  .version(false)
  .strict(true)
  .options({
    version: {
      type: 'string',
      describe: 'The version of react-native-windows to use.',
    },
    namespace: {
      type: 'string',
      describe:
        "The native project namespace. This should be expressed using dots as separators. i.e. 'Level1.Level2.Level3'. The generator will apply the correct syntax for the target language",
    },
    verbose: {
      type: 'boolean',
      describe: 'Enables logging.',
      default: false,
    },
    telemetry: {
      type: 'boolean',
      describe:
        'Controls sending telemetry that allows analysis of usage and failures of the react-native-windows CLI',
      default: false,
    },
    language: {
      type: 'string',
      describe: 'The language the project is written in.',
      choices: ['cs', 'cpp'],
      default: 'cpp',
    },
    overwrite: {
      type: 'boolean',
      describe: 'Overwrite any existing files without prompting',
      default: false,
    },
    projectType: {
      type: 'string',
      describe: 'The type of project to initialize.',
      choices: ['app', 'lib'],
      default: 'app',
    },
    experimentalNuGetDependency: {
      type: 'boolean',
      describe:
        '[Experimental] change to start consuming a NuGet containing a pre-built dll version of Microsoft.ReactNative',
      hidden: true,
      default: false,
    },
    useHermes: {
      type: 'boolean',
      describe:
        'Use Hermes instead of Chakra as the JS engine (supported on 0.64+)',
      default: false,
    },
    useWinUI3: {
      type: 'boolean',
      describe: '[Experimental] Use WinUI3',
      hidden: true,
      default: false,
    },
    nuGetTestVersion: {
      type: 'string',
      describe:
        '[internalTesting] By default the NuGet version matches the rnw package. This flag allows manually specifying the version for internal testing.',
      hidden: true,
    },
    nuGetTestFeed: {
      type: 'string',
      describe:
        '[internalTesting] Allows a test feed to be added to the generated NuGet configuration',
      hidden: true,
    },
    useDevMode: {
      type: 'boolean',
      describe:
        '[internalTesting] Link rather than Add/Install the react-native-windows package. This option is for the development workflow of the developers working on react-native-windows.',
      hidden: true,
      conflicts: 'version',
    },
  })
  .strict(true).argv;

if (argv.verbose) {
  console.log(argv);
}

if (!argv.telemetry) {
  if (argv.verbose) {
    console.log('Disabling telemetry');
  }
  Telemetry.disable();
} else {
  Telemetry.setup();
}

function getReactNativeProjectName(): string {
  console.log('Reading project name from package.json...');
  const cwd = process.cwd();
  const pkgJsonPath = findUp.sync('package.json', {cwd});
  if (!pkgJsonPath) {
    userError(
      'Unable to find package.json.  This should be run from within an existing react-native project.',
      'NO_PACKAGE_JSON',
    );
  }
  let name = JSON.parse(fs.readFileSync(pkgJsonPath, 'utf8')).name;
  if (!name) {
    const appJsonPath = findUp.sync('app.json', {cwd});
    if (appJsonPath) {
      console.log('Reading project name from app.json...');
      name = JSON.parse(fs.readFileSync(appJsonPath, 'utf8')).name;
    }
  }
  if (!name) {
    console.error('Please specify name in package.json or app.json');
  }
  return name;
}

function getReactNativeVersion(): string {
  console.log('Reading react-native version from node_modules...');
  const rnPkgJsonPath = require.resolve('react-native/package.json', {
    paths: [process.cwd()],
  });
  if (fs.existsSync(rnPkgJsonPath)) {
    return require(rnPkgJsonPath).version;
  }

  userError(
    'Must be run from a project that already depends on react-native, and has react-native installed.',
    'NO_REACTNATIVE_FOUND',
  );
}

function getDefaultReactNativeWindowsSemVerForReactNativeVersion(
  rnVersion: string,
): string {
  const validVersion = semver.valid(rnVersion);
  if (validVersion) {
    const major = semver.major(validVersion);
    const minor = semver.minor(validVersion);
    if (major === 0 && minor >= 59) {
      return `^${major}.${minor}.0-0`;
    }
  }

  userError(
    `Error: Unsupported version of react-native: ${chalk.cyan(
      rnVersion,
    )} react-native-windows supports react-native versions ${chalk.cyan(
      '>=0.60',
    )}`,
    'UNSUPPORTED_VERSION_RN',
  );
}

function getMatchingReactNativeSemVerForReactNativeWindowsVersion(
  rnwVersion: string,
): string {
  const validVersion = semver.valid(rnwVersion);
  if (validVersion) {
    const major = semver.major(validVersion);
    const minor = semver.minor(validVersion);
    if (major === 0 && minor >= 59) {
      return `^${major}.${minor}`;
    }
  }
  return 'unknown';
}

function getLatestMatchingVersion(
  pkg: string,
  versionSemVer: string,
): Promise<string> {
  return new Promise((resolve, reject) => {
    if (semver.validRange(versionSemVer)) {
      // Ideally we'd be able to just use npm.packages.range(pkg, versionSemVer) here,
      // but alas it fails to give us the right result for react-native-windows@^0.60.0-0
      // as it fails to return pre-release versions
      npm.packages.releases(
        pkg,
        (err: any, details: {[key: string]: Record<string, any>} | null) => {
          if (err) {
            reject(err);
          } else if (details) {
            const versions = Object.keys(details);
            if (versions.length > 0) {
              const candidates = versions
                .filter(v => semver.satisfies(v, versionSemVer))
                .sort(semver.rcompare);
              if (candidates.length > 0) {
                resolve(candidates[0]);
                return;
              }
            }
          }
          reject(
            new Error(`No matching version of ${pkg}@${versionSemVer} found`),
          );
        },
      );
    } else {
      try {
        exec(
          `npm info ${pkg}@${versionSemVer} version --json`,
          (err, stdout, _stderr) => {
            try {
              if (!err) {
                let candidates = JSON.parse(stdout);
                if (typeof candidates === 'string') {
                  resolve(candidates);
                  return;
                }
                candidates = candidates.sort(semver.rcompare);
                if (candidates && candidates.length > 0) {
                  resolve(candidates[0]);
                  return;
                }
              }
              reject(
                new Error(
                  `No matching version of ${pkg}@${versionSemVer} found`,
                ),
              );
            } catch (e) {
              reject(e);
            }
          },
        );
      } catch (err) {
        reject(
          new Error(`No matching version of ${pkg}@${versionSemVer} found`),
        );
      }
    }
  });
}

async function getLatestRNWVersion(): Promise<string> {
  const rnwLatestVersion = await getLatestMatchingRNWVersion('latest');
  if (!rnwLatestVersion) {
    userError(
      'Error: No version of react-native-windows@latest found',
      'NO_LATEST_RNW',
    );
  }
  return rnwLatestVersion;
}

async function getLatestMatchingRNWVersion(
  versionSemVer: string,
): Promise<string | null> {
  try {
    const version = await getLatestMatchingVersion(
      'react-native-windows',
      versionSemVer,
    );
    return version;
  } catch (err) {
    return null;
  }
}

function installReactNativeWindows(
  version: string | undefined,
  useDevMode: boolean,
) {
  const cwd = process.cwd();
  const execOptions = argv.verbose ? {stdio: 'inherit' as 'inherit'} : {};

  if (useDevMode) {
    const packageCmd = isProjectUsingYarn(cwd) ? 'yarn' : 'npm';
    execSync(`${packageCmd} link react-native-windows`, execOptions);
    version = '*';
  } else if (!version) {
    internalError(
      'Unexpected error encountered. If you are able, please file an issue on: https://github.com/microsoft/react-native-windows/issues/new/choose',
    );
  }

  console.log(
    `Installing ${chalk.green('react-native-windows')}@${chalk.cyan(
      version,
    )}...`,
  );

  const pkgJsonPath = findUp.sync('package.json', {cwd});
  if (!pkgJsonPath) {
    internalError('Unable to find package.json');
  }

  const pkgJson = require(pkgJsonPath);

  // check how react-native is installed
  if ('dependencies' in pkgJson && 'react-native' in pkgJson.dependencies) {
    // regular dependency (probably an app), inject into json and run install
    pkgJson.dependencies['react-native-windows'] = version;
    fs.writeFileSync(pkgJsonPath, JSON.stringify(pkgJson, null, 2));
    execSync(isProjectUsingYarn(cwd) ? 'yarn' : 'npm install', execOptions);
  } else if (
    'devDependencies' in pkgJson &&
    'react-native' in pkgJson.devDependencies
  ) {
    // only a dev dependency (probably a native module),
    execSync(
      isProjectUsingYarn(cwd)
        ? `yarn add react-native-windows@${version} --dev`
        : `npm install react-native-windows@${version} --save-dev`,
      execOptions,
    );
  } else {
    userError(
      "Unable to find 'react-native' in package.json's dependencies or devDependencies. This should be run from within an existing react-native app or lib.",
      'NO_REACTNATIVE_DEPENDENCIES',
    );
  }

  console.log(
    chalk.green(
      `react-native-windows@${chalk.cyan(
        require(require.resolve('react-native-windows/package.json', {
          paths: [cwd],
        })).version,
      )} successfully installed.`,
    ),
  );
}

function getRNWInitVersion(): string {
  try {
    const pkgJson = require('../package.json');
    if (
      pkgJson.name === 'react-native-windows-init' &&
      pkgJson.version !== undefined
    ) {
      return pkgJson.version;
    }
  } catch {}
  return '';
}

function setExit(exitCode: UserExitCode, error?: string): void {
  if (!process.exitCode || process.exitCode === exitCodes.SUCCESS) {
    Telemetry.client?.trackEvent({
      name: 'init-exit',
      properties: {
        durationInSecs: process.uptime(),
        msftInternal: isMSFTInternal(),
        exitCode: exitCode,
        rnwinitVersion: getRNWInitVersion(),
        errorMessage: error,
      },
    });
    process.exitCode = exitCodes[exitCode];
  }
}

/**
 * Throws a user or setup error
 */
function userError(text: string, exitCode: UserExitCode): never {
  throw new UserError(exitCode, text);
}

/**
 * Throw new internal error
 */
function internalError(text: string): never {
  throw new Error(text);
}

/**
 * Check if project is using Yarn (has `yarn.lock` in the tree)
 */
function isProjectUsingYarn(cwd: string): boolean {
  return !!findUp.sync('yarn.lock', {cwd});
}

(async () => {
  try {
    const name = getReactNativeProjectName();
    const ns = argv.namespace || name;
    const useDevMode = !!argv.useDevMode;
    let version = argv.version;

    if (argv.useWinUI3 && argv.experimentalNuGetDependency) {
      userError(
        "Error: Incompatible options specified. Options '--useWinUI3' and '--experimentalNuGetDependency' are incompatible",
        'INCOMPATIBLE_OPTIONS',
      );
    }

    if (argv.useHermes && argv.experimentalNuGetDependency) {
      userError(
        "Error: Incompatible options specified. Options '--useHermes' and '--experimentalNuGetDependency' are incompatible",
        'INCOMPATIBLE_OPTIONS',
      );
    }

    if (argv.useHermes && argv.language === 'cs') {
      userError(
        "Error: '--useHermes' is not yet compatible with C# projects",
        'INCOMPATIBLE_OPTIONS',
      );
    }

    if (!useDevMode) {
      if (!version) {
        const rnVersion = getReactNativeVersion();
        version = getDefaultReactNativeWindowsSemVerForReactNativeVersion(
          rnVersion,
        );
      }

      const rnwResolvedVersion = await getLatestMatchingRNWVersion(version);

      if (!rnwResolvedVersion) {
        if (argv.version) {
          console.warn(
            `Warning: Querying npm to find react-native-windows@${argv.version} failed.  Attempting to continue anyway...`,
          );
        } else {
          const rnwLatestVersion = await getLatestRNWVersion();
          userError(
            `
  No compatible version of ${chalk.green('react-native-windows')} found.
  The latest supported version is ${chalk.green(
    'react-native-windows',
  )}@${chalk.cyan(rnwLatestVersion)}.
  Please modify your application to use ${chalk.green(
    'react-native',
  )}@${chalk.cyan(
              getMatchingReactNativeSemVerForReactNativeWindowsVersion(
                rnwLatestVersion,
              ),
            )} or another supported version of ${chalk.green(
              'react-native',
            )} and try again.
  `,
            'NO_AUTO_MATCHING_RNW',
          );
        }
      }

      if (!argv.version) {
        console.log(
          `Latest matching version of ${chalk.bold(
            'react-native-windows',
          )} for ${chalk.green('react-native')}@${chalk.cyan(
            getReactNativeVersion(),
          )} is ${chalk.green('react-native-windows')}@${chalk.cyan(
            rnwResolvedVersion,
          )}`,
        );

        if (rnwResolvedVersion && semver.prerelease(rnwResolvedVersion)) {
          const rnwLatestVersion = await getLatestRNWVersion();
          console.warn(
            `
  ${chalk.green('react-native-windows')}@${chalk.cyan(
              rnwResolvedVersion,
            )} is a ${chalk.yellow('pre-release')} version.
  The latest supported version is ${chalk.green(
    'react-native-windows',
  )}@${chalk.cyan(rnwLatestVersion)}.
  You can either downgrade your version of ${chalk.green(
    'react-native',
  )} to ${chalk.cyan(
              getMatchingReactNativeSemVerForReactNativeWindowsVersion(
                rnwLatestVersion,
              ),
            )}, or continue with a ${chalk.yellow(
              'pre-release',
            )} version of ${chalk.bold('react-native-windows')}.
  `,
          );

          const confirm: boolean = (
            await prompts({
              type: 'confirm',
              name: 'confirm',
              message: `Do you wish to continue with ${chalk.green(
                'react-native-windows',
              )}@${chalk.cyan(rnwResolvedVersion)}?`,
            })
          ).confirm;

          if (!confirm) {
            userError('User canceled', 'USER_CANCEL');
          }
        }
      }
    }

    installReactNativeWindows(version, useDevMode);

    const generateWindows = requireGenerateWindows();
    await generateWindows(process.cwd(), name, ns, {
      language: argv.language as 'cs' | 'cpp',
      overwrite: argv.overwrite,
      verbose: argv.verbose,
      projectType: argv.projectType as 'lib' | 'app',
      experimentalNuGetDependency: argv.experimentalNuGetDependency,
      useWinUI3: argv.useWinUI3,
      useHermes: argv.useHermes,
      useDevMode: useDevMode,
      nuGetTestVersion: argv.nuGetTestVersion,
      nuGetTestFeed: argv.nuGetTestFeed,
      telemetry: argv.telemetry,
    });
    return setExit('SUCCESS');
  } catch (error) {
    const exitCode =
      error instanceof UserError
        ? (error as UserError).exitCode
        : 'UNKNOWN_ERROR';
    if (exitCode !== 'SUCCESS') {
      console.error(chalk.red(error.message));
      console.error(error);
    }
    setExit(exitCode, error.message);
  } finally {
    Telemetry.client?.flush();
  }
})();
