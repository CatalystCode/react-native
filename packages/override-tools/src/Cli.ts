#!/usr/bin/env node

/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import * as FileRepository from './FileRepository';
import * as Manifest from './Manifest';

import * as chalk from 'chalk';
import * as fs from 'fs';
import * as ora from 'ora';
import * as path from 'path';
import * as yargs from 'yargs';

import CrossProcessLock from './CrossProcessLock';
import GitReactFileRepository from './GitReactFileRepository';
import OverrideFileRepositoryImpl from './OverrideFileRepositoryImpl';

import {getInstalledRNVersion} from './PackageUtils';

doMain(() => {
  yargs
    .command(
      'validate <manifest> [version]',
      'Verify that overrides are recorded and up-to-date',
      cmdYargs =>
        cmdYargs.options({
          manifest: {type: 'string', describe: 'Path to an override manifest'},
          version: {type: 'string', describe: 'React Native Version'},
        }),
      cmdArgv => validateManifest(cmdArgv.manifest, cmdArgv.version),
    )
    .epilogue(
      'This tool allows managing JavaScript overrides for React Native Windows',
    )
    .demandCommand()
    .recommendCommands()
    .strict()
    .showHelpOnFail(false)
    .wrap(yargs.terminalWidth())
    .version(false)
    .scriptName('override')
    .argv;
});

/**
 * Check that the manifest correctly describes files and is up to date
 */
async function validateManifest(manifestPath: string, version?: string) {
  try {
    await fs.promises.access(manifestPath);
  } catch (ex) {
    console.error(
      chalk.red(`Could not find manifest file at path '${manifestPath}'`),
    );
    throw ex;
  }

  const spinner = ora();

  try {
    spinner.start('Initializing local React Native repo');
    const [reactRepo, ovrRepo] = await getFileRepos(manifestPath);
    spinner.succeed();

    let manifest: Manifest.Manifest;

    try {
      manifest = await Manifest.readFromFile(manifestPath);
    } catch (ex) {
      console.error(chalk.red('Could not parse manifest. Is it valid?'));
      throw ex;
    }

    spinner.start('Verifying overrides are recorded and up to date');
    const rnVersion = version || (await getInstalledRNVersion(manifestPath));
    const validationErrors = await Manifest.validate(
      manifest,
      ovrRepo,
      FileRepository.bindVersion(reactRepo, rnVersion),
    );

    if (validationErrors.length === 0) {
      spinner.succeed();
    } else {
      spinner.fail();
      printValidationErrors(validationErrors);
      process.exitCode = 1;
    }
  } catch (ex) {
    if (spinner.isSpinning) {
      spinner.fail();
    }
    throw ex;
  }
}

/**
 * Create and initialize file repos
 */
async function getFileRepos(
  manifestPath: string,
): Promise<[GitReactFileRepository, OverrideFileRepositoryImpl]> {
  const reactRepo = await GitReactFileRepository.createAndInit();

  const ovrPath = path.dirname(manifestPath);
  const ovrFilter = /^.*\.(js|ts|jsx|tsx)$/;
  const ovrRepo = new OverrideFileRepositoryImpl(ovrPath, ovrFilter);

  return [reactRepo, ovrRepo];
}

/**
 * Prints validation errors in a user-readable form to stderr
 */
function printValidationErrors(errors: Array<Manifest.ValidationError>) {
  errors.sort((a, b) => a.file.localeCompare(b.file));

  const filesMissing = errors.filter(err => {
    return err.type === 'fileMissingFromManifest';
  });
  const overridesMissing = errors.filter(err => {
    return err.type === 'overrideFileNotFound';
  });
  const baseFilesNotFound = errors.filter(err => {
    return err.type === 'baseFileNotFound';
  });
  const outOfDateFiles = errors.filter(err => {
    return err.type === 'outOfDate';
  });

  if (filesMissing.length > 0) {
    // TODO: Instruct users to use 'yarn override add' once that exists
    console.error(
      chalk.red("\nFound override files that aren't listed in the manifest:"),
    );
    filesMissing.forEach(err => console.error(` - ${err.file}`));
  }

  if (overridesMissing.length > 0) {
    // TODO: Instruct users to use 'yarn override remove' once that exists
    console.error(
      chalk.red("\nFound overrides in the manifest that don't exist on disk:"),
    );
    overridesMissing.forEach(err => console.error(` - ${err.file}`));
  }

  if (baseFilesNotFound.length > 0) {
    // TODO: Instruct users to use 'yarn override remove' once that exists
    console.error(
      chalk.red('\nFound overrides whose original files do not exist:'),
    );
    baseFilesNotFound.forEach(err => console.error(` - ${err.file}`));
  }

  if (outOfDateFiles.length > 0) {
    // TODO: Instruct users to use 'yarn override upgrade' once that exists
    console.error(
      chalk.red('\nFound overrides whose original files have changed:'),
    );
    outOfDateFiles.forEach(err => console.error(` - ${err.file}`));
  }
}

/**
 * Wrap the main function around a barrier to ensure only one copy of the
 * override tool is running at once. This is needed to avoid multiple tools
 * accessing the same local Git repo at the same time.
 */
async function doMain(fn: () => void): Promise<void> {
  const lock = new CrossProcessLock('react-native-windows-override-cli-lock');

  if (!(await lock.tryLock())) {
    const spinner = ora(
      'Waiting for other instances of the override CLI to finish',
    ).start();
    await lock.lock();
    spinner.stop();
  }

  fn();
  lock.unlock();
}
