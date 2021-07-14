/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import yargs from 'yargs';
import fs from 'fs';
import semver from 'semver';
import {execSync} from 'child_process';
import validUrl from 'valid-url';
import prompts from 'prompts';
import findUp from 'find-up';
import chalk from 'chalk';
import npmFetch from 'npm-registry-fetch';

import {
  Telemetry,
  isMSFTInternal,
  CodedErrorType,
  CodedErrors,
  CodedError,
} from '@react-native-windows/telemetry';

/**
 * Important:
 * Do not use process.exit() in this script as it will prevent telemetry from being sent.
 * See https://github.com/microsoft/ApplicationInsights-node.js/issues/580
 */

import {
  registerCliOptions as registerGeneratorCliOptions,
  requireGenerateProject,
} from './GeneratorShim';

const npmConfReg = execSync('npm config get registry')
  .toString()
  .trim();
const NPM_REGISTRY_URL = validUrl.isUri(npmConfReg)
  ? npmConfReg
  : 'http://registry.npmjs.org';

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
    useDevMode: {
      type: 'boolean',
      describe:
        '[internalTesting] Link rather than Add/Install the react-native-windows package. This option is for the development workflow of the developers working on react-native-windows.',
      hidden: true,
      conflicts: 'version',
    },
  }).argv;

registerGeneratorCliOptions(yargs);

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
    throw new CodedError(
      'NoPackageJSon',
      'Unable to find package.json.  This should be run from within an existing react-native project.',
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

  throw new CodedError(
    'NoReactNativeFound',
    'Must be run from a project that already depends on react-native, and has react-native installed.',
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

  throw new CodedError(
    'UnsupportedReactNativeVersion',
    `Error: Unsupported version of react-native: ${chalk.cyan(
      rnVersion,
    )} react-native-windows supports react-native versions ${chalk.cyan(
      '>=0.60',
    )}`,
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

async function getLatestMatchingVersion(
  pkg: string,
  versionSemVer: string,
): Promise<string> {
  const npmResponse = await npmFetch.json(pkg, {registry: NPM_REGISTRY_URL});

  // Check if versionSemVer is a tag (i.e. 'canary', 'latest', 'preview', etc.)
  if ('dist-tags' in npmResponse) {
    const distTags = npmResponse['dist-tags'] as Record<string, string>;
    if (versionSemVer in distTags) {
      return distTags[versionSemVer];
    }
  }

  // Check if versionSemVer is a semver version (i.e. '^0.60.0-0', '0.63.1', etc.)
  if ('versions' in npmResponse) {
    const versions = Object.keys(
      npmResponse.versions as Record<string, unknown>,
    );
    if (versions.length > 0) {
      const candidates = versions
        .filter(v => semver.satisfies(v, versionSemVer))
        .sort(semver.rcompare);
      if (candidates.length > 0) {
        return candidates[0];
      }
    }
  }

  throw new CodedError(
    'NoMatchingPackageVersion',
    `No matching version of ${pkg}@${versionSemVer} found`,
  );
}

async function getLatestRNWVersion(): Promise<string> {
  const rnwLatestVersion = await getLatestMatchingRNWVersion('latest');
  if (!rnwLatestVersion) {
    throw new CodedError(
      'NoLatestReactNativeWindows',
      'Error: No version of react-native-windows@latest found',
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
    throw new CodedError(
      'Unknown',
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
    throw new CodedError('NoPackageJSon', 'Unable to find package.json');
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
    throw new CodedError(
      'NoReactNativeDependencies',
      "Unable to find 'react-native' in package.json's dependencies or devDependencies. This should be run from within an existing react-native app or lib.",
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

function setExit(exitCode: CodedErrorType, error?: string): void {
  if (!process.exitCode || process.exitCode === CodedErrors.Success) {
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
    process.exitCode = CodedErrors[exitCode];
  }
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
      throw new CodedError(
        'IncompatibleOptions',
        "Error: Incompatible options specified. Options '--useWinUI3' and '--experimentalNuGetDependency' are incompatible",
        {detail: 'useWinUI3 and experimentalNuGetDependency'},
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
          throw new CodedError(
            'NoAutoMatchingReactNativeWindows',
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
            {rnwLatestVersion: rnwLatestVersion},
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
            throw new CodedError('UserCancel', 'User canceled');
          }
        }
      }
    }

    installReactNativeWindows(version, useDevMode);

    const generateProject = requireGenerateProject();
    if (generateProject === null) {
      throw new CodedError(
        'GeneratorNotFound',
        'Could not locate project generator',
      );
    }

    await generateProject(process.cwd(), name, ns, {
      useDevMode,
      verbose: argv.verbose,
    });
    return setExit('Success');
  } catch (error) {
    const exitCode =
      error instanceof CodedError
        ? ((error as CodedError).name as CodedErrorType)
        : 'Unknown';
    if (exitCode !== 'Success') {
      console.error(chalk.red(error.message));
      console.error(error);
    }
    setExit(exitCode, error.message);
  } finally {
    Telemetry.client?.flush();
  }
})();
