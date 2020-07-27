/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import * as build from './utils/build';
import * as chalk from 'chalk';
import * as deploy from './utils/deploy';
import {newError, newInfo} from './utils/commandWithProgress';
import * as info from './utils/info';
import MSBuildTools from './utils/msbuildtools';

import {Command, Config} from '@react-native-community/cli-types';
import {runWindowsOptions, RunWindowsOptions} from './runWindowsOptions';

import autolink = require('./utils/autolink');

function ExitProcessWithError(loggingWasEnabled: boolean): never {
  if (!loggingWasEnabled) {
    console.log(
      `Re-run the command with ${chalk.bold('--logging')} for more information`,
    );
  }
  process.exit(1);
}

/**
 * Performs build deploy and launch of RNW apps.
 * @param args Unprocessed args passed from react-native CLI.
 * @param config Config passed from react-native CLI.
 * @param options Options passed from react-native CLI.
 */
async function runWindows(
  args: string[],
  config: Config,
  options: RunWindowsOptions,
) {
  const verbose = options.logging;

  if (verbose) {
    newInfo('Verbose: ON');
  }

  if (options.info) {
    try {
      const output = await info.getEnvironmentInfo();
      console.log(output.trimEnd());
      console.log('  Installed UWP SDKs:');
      const sdks = MSBuildTools.getAllAvailableUAPVersions();
      sdks.forEach(version => console.log('    ' + version));
      return;
    } catch (e) {
      newError('Unable to print environment info.\n' + e.toString());
      ExitProcessWithError(options.logging);
    }
  }

  // Get the solution file
  const slnFile = build.getAppSolutionFile(options, config);

  if (options.autolink) {
    const autolinkArgs: string[] = [];
    const autolinkConfig = config;
    const autoLinkOptions = {
      logging: options.logging,
      proj: options.proj,
      sln: options.sln,
    };
    await autolink.func(autolinkArgs, autolinkConfig, autoLinkOptions);
  } else {
    newInfo('Autolink step is skipped');
  }

  const buildTools = MSBuildTools.findAvailableVersion(options.arch, verbose);

  if (options.build) {
    if (!slnFile) {
      newError(
        'Visual Studio Solution file not found. Maybe run "react-native windows" first?',
      );
      ExitProcessWithError(options.logging);
    }

    try {
      await build.restoreNuGetPackages(slnFile, buildTools, verbose);
    } catch (e) {
      newError('Failed to restore the NuGet packages: ' + e.toString());
      ExitProcessWithError(options.logging);
    }

    // Get build/deploy options
    const buildType = deploy.getBuildConfiguration(options);
    var msBuildProps = build.parseMsBuildProps(options);

    if (!options.autolink) {
      // Disable the autolink check if --no-autolink was passed
      msBuildProps.RunAutolinkCheck = 'false';
    }

    try {
      await build.buildSolution(
        buildTools,
        slnFile,
        buildType,
        options.arch,
        msBuildProps,
        verbose,
        undefined, // build the default target
        options.buildLogDirectory,
      );
    } catch (e) {
      newError(
        `Build failed with message ${
          e.message
        }. Check your build configuration.`,
      );
      if (e.logfile) {
        console.log('See', chalk.bold(e.logfile));
      }
      ExitProcessWithError(options.logging);
    }
  } else {
    newInfo('Build step is skipped');
  }

  await deploy.startServerInNewWindow(options, verbose);

  if (options.deploy) {
    if (!slnFile) {
      newError(
        'Visual Studio Solution file not found. Maybe run "react-native windows" first?',
      );
      ExitProcessWithError(options.logging);
    }

    try {
      if (options.device || options.emulator || options.target) {
        await deploy.deployToDevice(options, verbose);
      } else {
        await deploy.deployToDesktop(options, verbose, config, buildTools);
      }
    } catch (e) {
      newError(`Failed to deploy${e ? `: ${e.message}` : ''}`);
      ExitProcessWithError(options.logging);
    }
  } else {
    newInfo('Deploy step is skipped');
  }
}

/*
// Example of running the Windows Command
runWindows({
  root: 'C:\\github\\hack\\myapp',
  debug: true,
  arch: 'x86',
  nugetPath: 'C:\\github\\react\\react-native-windows\\local-cli\\runWindows\\.nuget\\nuget.exe'
});
*/

/**
 * Starts the app on a connected Windows emulator or mobile device.
 */
const runWindowsCommand: Command = {
  name: 'run-windows',
  description:
    'builds your app and starts it on a connected Windows desktop, emulator or device',
  func: runWindows,
  options: runWindowsOptions,
};

export = runWindowsCommand;
