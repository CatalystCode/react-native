/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import fs from 'fs';
import path from 'path';
import yargs from 'yargs';
import {Telemetry} from '@react-native-windows/telemetry';

import {
  copyProjectTemplateAndReplace,
  installScriptsAndDependencies,
} from './generator-windows';

import {autoLinkCommand} from './runWindows/utils/autolink';
import {runWindowsCommand} from './runWindows/runWindows';
import {dependencyConfigWindows} from './config/dependencyConfig';
import {projectConfigWindows} from './config/projectConfig';
import {
  GeneratorOptions,
  GeneratorCliOptions,
  GeneratorInitOptions,
  registerCliOptions,
} from './generatorOptions';

export {registerCliOptions};
/**
 * Simple utility for running the Windows generator.
 *
 * @param  projectDir root project directory (i.e. contains index.js)
 * @param  name       name of the root JS module for this app
 * @param  ns         namespace for the project
 * @param  options    non-CLI-driven generation options
 */
export async function generateProject(
  projectDir: string,
  name: string,
  ns: string,
  initOptions: GeneratorInitOptions,
) {
  const cliOptions = (yargs.argv as unknown) as GeneratorCliOptions;
  const options: GeneratorOptions = {...initOptions, ...cliOptions};

  let error;
  try {
    if (!fs.existsSync(projectDir)) {
      fs.mkdirSync(projectDir);
    }
    await installScriptsAndDependencies({verbose: options.verbose});

    const rnwPackage = path.dirname(
      require.resolve('react-native-windows/package.json', {
        paths: [projectDir],
      }),
    );
    const templateRoot = path.join(rnwPackage, 'template');
    await copyProjectTemplateAndReplace(
      templateRoot,
      projectDir,
      name,
      ns,
      options,
    );
  } catch (e) {
    error = e;
    Telemetry.trackException(error);
    throw e;
  } finally {
    if (Telemetry.client) {
      let rnVersion = '';
      let cliVersion = '';
      try {
        const cwd = process.cwd();
        const rnwPkg = JSON.parse(
          fs
            .readFileSync(
              require.resolve('react-native-windows/package.json', {
                paths: [cwd],
              }),
            )
            .toString(),
        );
        rnVersion = rnwPkg.peerDependencies['react-native'] || '';
        const rnwCliPkgJson = require('../package.json');
        cliVersion = rnwCliPkgJson.version;
      } catch {}
      const optScrubbed = scrubOptions(options);
      Telemetry.client.trackEvent({
        name: 'generate-windows',
        properties: {
          error: error,
          ...optScrubbed,
          'react-native': rnVersion,
          'cli-version': cliVersion,
        },
      });

      Telemetry.client.flush();
    }
  }
}

export type GenerateProject = typeof generateProject;

function scrubOptions(opts: GeneratorOptions) {
  return {
    overwrite: opts.overwrite,
    language: opts.language,
    projectType: opts.projectType,
    experimentalNuGetDependency: opts.experimentalNuGetDependency,
    nuGetTestFeed: opts.nuGetTestFeed !== undefined ? true : false,
    nuGetTestVersion: opts.nuGetTestVersion !== undefined ? true : false,
    useWinUI3: opts.useWinUI3,
    useHermes: opts.useHermes,
    verbose: opts.verbose,
  };
}

export const commands = [autoLinkCommand, runWindowsCommand];
export const dependencyConfig = dependencyConfigWindows;
export const projectConfig = projectConfigWindows;
export * from './healthChecks';
