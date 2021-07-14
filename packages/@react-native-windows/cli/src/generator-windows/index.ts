/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import chalk from 'chalk';
import path from 'path';
import username from 'username';
import uuid from 'uuid';
import childProcess from 'child_process';
import fs from 'fs';
import os from 'os';
import semver from 'semver';
import _ from 'lodash';
import findUp from 'find-up';
import {readProjectFile, findPropertyValue} from '../config/configUtils';

import {
  createDir,
  copyAndReplaceAll,
  copyAndReplaceWithChangedCallback,
} from '../generator-common';
import {GeneratorOptions} from '../generatorOptions';
import {CodedError} from '@react-native-windows/telemetry';
import {
  findPackage,
  WritableNpmPackage,
} from '@react-native-windows/package-utils';

const windowsDir = 'windows';
const bundleDir = 'Bundle';

async function generateCertificate(
  srcPath: string,
  destPath: string,
  newProjectName: string,
  currentUser: string,
): Promise<string | null> {
  console.log('Generating self-signed certificate...');
  if (os.platform() === 'win32') {
    try {
      const thumbprint = childProcess
        .execSync(
          `powershell -NoProfile -Command "Write-Output (New-SelfSignedCertificate -KeyUsage DigitalSignature -KeyExportPolicy Exportable -Subject 'CN=${currentUser}' -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}Subject Type:End Entity') -CertStoreLocation 'Cert:\\CurrentUser\\My').Thumbprint"`,
        )
        .toString()
        .trim();
      if (!fs.existsSync(path.join(windowsDir, newProjectName))) {
        fs.mkdirSync(path.join(windowsDir, newProjectName));
      }
      childProcess.execSync(
        `powershell -NoProfile -Command "$pwd = (ConvertTo-SecureString -String password -Force -AsPlainText); Export-PfxCertificate -Cert 'cert:\\CurrentUser\\My\\${thumbprint}' -FilePath ${path.join(
          windowsDir,
          newProjectName,
          newProjectName,
        )}_TemporaryKey.pfx -Password $pwd"`,
      );
      console.log(
        chalk.green('Self-signed certificate generated successfully.'),
      );
      return thumbprint;
    } catch (err) {
      console.log(
        chalk.yellow('Unable to generate the self-signed certificate:'),
      );
      console.log(chalk.red(err));
    }
  }

  console.log(
    chalk.yellow('Using Default Certificate. Use Visual Studio to renew it.'),
  );
  await copyAndReplaceWithChangedCallback(
    path.join(srcPath, 'keys', 'MyApp_TemporaryKey.pfx'),
    destPath,
    path.join(windowsDir, newProjectName, newProjectName + '_TemporaryKey.pfx'),
  );

  return null;
}

/**
 * This represents the data to insert nuget packages
 */
interface NugetPackage {
  id: string;
  version: string;
}

/**
 * This represents the data to insert nuget packages with Cpp specific information
 */
interface CppNugetPackage extends NugetPackage {
  propsTopOfFile?: boolean;
  propsMiddleOfFile?: boolean;
  hasProps: boolean;
  hasTargets: boolean;
}

function pascalCase(str: string) {
  const camelCase = _.camelCase(str);
  return camelCase[0].toUpperCase() + camelCase.substr(1);
}

function resolveRnwPath(subpath: string): string {
  return require.resolve(path.join('react-native-windows', subpath), {
    paths: [process.cwd()],
  });
}

// Existing high cyclomatic complexity
// eslint-disable-next-line complexity
export async function copyProjectTemplateAndReplace(
  srcRootPath: string,
  destPath: string,
  newProjectName: string,
  namespace: string,
  options: GeneratorOptions,
) {
  if (!srcRootPath) {
    throw new CodedError(
      'CopyProjectTemplateNoSourcePath',
      'Need a path to copy from',
    );
  }

  if (!destPath) {
    throw new CodedError(
      'CopyProjectTemplateNoDestPath',
      'Need a path to copy to',
    );
  }

  if (!newProjectName) {
    throw new CodedError(
      'CopyProjectTemplateNoProjectName',
      'Need a project name',
    );
  }

  const projectType = options.projectType;

  // React-native init only allows alphanumerics in project names, but other
  // new project tools (like create-react-native-module) are less strict.
  if (projectType === 'lib') {
    newProjectName = pascalCase(newProjectName);
  }

  // Similar to the above, but we want to retain namespace separators
  if (projectType === 'lib') {
    namespace = namespace
      .split(/[.:]+/)
      .map(pascalCase)
      .join('.');
  }

  createDir(path.join(destPath, windowsDir));
  createDir(path.join(destPath, windowsDir, newProjectName));

  if (projectType === 'app') {
    createDir(path.join(destPath, windowsDir, newProjectName, bundleDir));
    createDir(path.join(destPath, windowsDir, newProjectName, 'BundleBuilder'));
  }

  const language = options.language;
  const namespaceCpp = toCppNamespace(namespace);
  if (options.experimentalNuGetDependency) {
    console.log('Using experimental NuGet dependency.');
  }
  if (options.useWinUI3) {
    console.log('Using experimental WinUI3 dependency.');
  }
  const projDir = 'proj';
  const srcPath = path.join(srcRootPath, `${language}-${projectType}`);
  const sharedPath = path.join(srcRootPath, `shared-${projectType}`);
  const projectGuid = uuid.v4();
  const rnwVersion = require(resolveRnwPath('package.json')).version;
  const nugetVersion = options.nuGetTestVersion || rnwVersion;
  const packageGuid = uuid.v4();
  const currentUser = username.sync()!; // Gets the current username depending on the platform.

  let mainComponentName = newProjectName;
  const appJsonPath = await findUp('app.json', {cwd: destPath});
  if (appJsonPath) {
    mainComponentName = JSON.parse(fs.readFileSync(appJsonPath, 'utf8')).name;
  }

  const certificateThumbprint =
    projectType === 'app'
      ? await generateCertificate(
          srcPath,
          destPath,
          newProjectName,
          currentUser,
        )
      : null;

  const xamlNamespace = options.useWinUI3
    ? 'Microsoft.UI.Xaml'
    : 'Windows.UI.Xaml';
  const xamlNamespaceCpp = toCppNamespace(xamlNamespace);

  const winuiPropsPath = resolveRnwPath('PropertySheets/WinUI.props');
  const winuiProps = readProjectFile(winuiPropsPath);
  const winui3Version = findPropertyValue(
    winuiProps,
    'WinUI3Version',
    winuiPropsPath,
  );
  const winui2xVersion = findPropertyValue(
    winuiProps,
    'WinUI2xVersion',
    winuiPropsPath,
  );

  const jsEnginePropsPath = resolveRnwPath('PropertySheets/JSengine.props');
  const hermesVersion = findPropertyValue(
    readProjectFile(jsEnginePropsPath),
    'HermesVersion',
    jsEnginePropsPath,
  );

  const csNugetPackages: NugetPackage[] = [
    {
      id: 'Microsoft.NETCore.UniversalWindowsPlatform',
      version: '6.2.9',
    },
    {
      id: 'ReactNative.Hermes.Windows',
      version: hermesVersion,
    },
  ];

  const cppNugetPackages: CppNugetPackage[] = [
    {
      id: 'Microsoft.Windows.CppWinRT',
      version: '2.0.210312.4',
      propsTopOfFile: true,
      hasProps: true,
      hasTargets: true,
    },
    {
      id: 'ReactNative.Hermes.Windows',
      version: hermesVersion,
      hasProps: false,
      hasTargets: true,
    },
  ];

  if (options.experimentalNuGetDependency) {
    csNugetPackages.push({
      id: 'Microsoft.ReactNative.Managed',
      version: nugetVersion,
    });

    cppNugetPackages.push({
      id: 'Microsoft.ReactNative',
      version: nugetVersion,
      hasProps: false,
      hasTargets: true,
    });

    cppNugetPackages.push({
      id: 'Microsoft.ReactNative.Cxx',
      version: nugetVersion,
      hasProps: false,
      hasTargets: true,
    });
  }

  const packagesConfigCppNugetPackages = [
    ...cppNugetPackages,
    {
      id: options.useWinUI3 ? 'Microsoft.WinUI' : 'Microsoft.UI.Xaml',
      version: options.useWinUI3 ? winui3Version : winui2xVersion,
      hasProps: false, // WinUI/MUX props and targets get handled by RNW's WinUI.props.
      hasTargets: false,
    },
  ];

  const templateVars: Record<string, any> = {
    useMustache: true,
    regExpPatternsToRemove: [],

    name: newProjectName,
    namespace: namespace,
    namespaceCpp: namespaceCpp,
    languageIsCpp: language === 'cpp',

    mainComponentName: mainComponentName,

    // Visual Studio is very picky about the casing of the guids for projects, project references and the solution
    // https://www.bing.com/search?q=visual+studio+project+guid+casing&cvid=311a5ad7f9fc41089507b24600d23ee7&FORM=ANAB01&PC=U531
    // we therefore have to precariously use the right casing in the right place or risk building in VS breaking.
    projectGuidLower: `{${projectGuid.toLowerCase()}}`,
    projectGuidUpper: `{${projectGuid.toUpperCase()}}`,

    // packaging and signing variables:
    packageGuid: packageGuid,
    currentUser: currentUser,
    certificateThumbprint: certificateThumbprint,

    useExperimentalNuget: options.experimentalNuGetDependency,
    nuGetTestFeed: options.nuGetTestFeed,

    // cpp template variables
    useWinUI3: options.useWinUI3,
    useHermes: options.useHermes,
    xamlNamespace: xamlNamespace,
    xamlNamespaceCpp: xamlNamespaceCpp,
    cppNugetPackages: cppNugetPackages,
    packagesConfigCppNugetPackages: packagesConfigCppNugetPackages,

    // cs template variables
    csNugetPackages: csNugetPackages,

    // autolinking template variables
    autolinkPropertiesForProps: '',
    autolinkProjectReferencesForTargets: '',
    autolinkCsUsingNamespaces: '',
    autolinkCsReactPackageProviders: '',
    autolinkCppIncludes: '',
    autolinkCppPackageProviders:
      '\n    UNREFERENCED_PARAMETER(packageProviders);', // CODESYNC: vnext\local-cli\runWindows\utils\autolink.js
  };

  const commonMappings =
    projectType === 'app'
      ? [
          // app common mappings
          {
            from: path.join(
              srcRootPath,
              options.useDevMode
                ? 'metro.devMode.config.js'
                : 'metro.config.js',
            ),
            to: 'metro.config.js',
          },
          {
            from: path.join(srcRootPath, '_gitignore'),
            to: path.join(windowsDir, '.gitignore'),
          },
          {
            from: path.join(srcRootPath, 'b_gitignore'),
            to: path.join(windowsDir, newProjectName, '.gitignore'),
          },
          {
            from: path.join(srcRootPath, 'index.windows.bundle'),
            to: path.join(
              windowsDir,
              newProjectName,
              bundleDir,
              'index.windows.bundle',
            ),
          },
          {
            from: path.join(srcPath, projDir, 'MyApp.sln'),
            to: path.join(windowsDir, newProjectName + '.sln'),
          },
        ]
      : [
          // lib common mappings
          {
            from: path.join(srcRootPath, '_gitignore'),
            to: path.join(windowsDir, '.gitignore'),
          },
          {
            from: path.join(srcPath, projDir, 'MyLib.sln'),
            to: path.join(windowsDir, newProjectName + '.sln'),
          },
        ];

  for (const mapping of commonMappings) {
    await copyAndReplaceWithChangedCallback(
      mapping.from,
      destPath,
      mapping.to,
      templateVars,
      options.overwrite,
    );
  }

  if (language === 'cs') {
    const csMappings =
      projectType === 'app'
        ? [
            // cs app mappings
            {
              from: path.join(srcPath, projDir, 'MyApp.csproj'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.csproj',
              ),
            },
          ]
        : [
            // cs lib mappings
            {
              from: path.join(srcPath, projDir, 'MyLib.csproj'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.csproj',
              ),
            },
          ];

    for (const mapping of csMappings) {
      await copyAndReplaceWithChangedCallback(
        mapping.from,
        destPath,
        mapping.to,
        templateVars,
        options.overwrite,
      );
    }
  } else {
    const cppMappings =
      projectType === 'app'
        ? [
            // cpp app mappings
            {
              from: path.join(srcPath, projDir, 'MyApp.vcxproj'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.vcxproj',
              ),
            },
            {
              from: path.join(srcPath, projDir, 'MyApp.vcxproj.filters'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.vcxproj.filters',
              ),
            },
            {
              from: path.join(srcPath, projDir, 'packages.config'),
              to: path.join(windowsDir, newProjectName, 'packages.config'),
            },
          ]
        : [
            // cpp lib mappings
            {
              from: path.join(srcPath, projDir, 'MyLib.vcxproj'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.vcxproj',
              ),
            },
            {
              from: path.join(srcPath, projDir, 'MyLib.vcxproj.filters'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.vcxproj.filters',
              ),
            },
            {
              from: path.join(srcPath, projDir, 'MyLib.def'),
              to: path.join(
                windowsDir,
                newProjectName,
                newProjectName + '.def',
              ),
            },
            {
              from: path.join(srcPath, projDir, 'packages.config'),
              to: path.join(windowsDir, newProjectName, 'packages.config'),
            },
          ];

    for (const mapping of cppMappings) {
      await copyAndReplaceWithChangedCallback(
        mapping.from,
        destPath,
        mapping.to,
        templateVars,
        options.overwrite,
      );
    }
  }

  // shared proj
  if (fs.existsSync(path.join(sharedPath, projDir))) {
    const sharedProjMappings = [];

    sharedProjMappings.push({
      from: path.join(sharedPath, projDir, 'NuGet.Config'),
      to: path.join(windowsDir, 'NuGet.Config'),
    });

    if (
      fs.existsSync(
        path.join(sharedPath, projDir, 'ExperimentalFeatures.props'),
      )
    ) {
      sharedProjMappings.push({
        from: path.join(sharedPath, projDir, 'ExperimentalFeatures.props'),
        to: path.join(windowsDir, 'ExperimentalFeatures.props'),
      });
    }

    for (const mapping of sharedProjMappings) {
      await copyAndReplaceWithChangedCallback(
        mapping.from,
        destPath,
        mapping.to,
        templateVars,
        options.overwrite,
      );
    }
  }

  // shared assets
  if (fs.existsSync(path.join(sharedPath, 'assets'))) {
    await copyAndReplaceAll(
      path.join(sharedPath, 'assets'),
      destPath,
      path.join(windowsDir, newProjectName, 'Assets'),
      templateVars,
      options.overwrite,
    );
  }

  // shared src
  if (fs.existsSync(path.join(sharedPath, 'src'))) {
    await copyAndReplaceAll(
      path.join(sharedPath, 'src'),
      destPath,
      path.join(windowsDir, newProjectName),
      templateVars,
      options.overwrite,
    );
  }

  // src
  if (fs.existsSync(path.join(srcPath, 'src'))) {
    await copyAndReplaceAll(
      path.join(srcPath, 'src'),
      destPath,
      path.join(windowsDir, newProjectName),
      templateVars,
      options.overwrite,
    );
  }

  if (projectType === 'app') {
    console.log(chalk.white.bold('To run your app on UWP:'));
    console.log(chalk.white('   npx react-native run-windows'));
  }
}

function toCppNamespace(namespace: string) {
  return namespace.replace(/\./g, '::');
}

export async function installScriptsAndDependencies(options: {
  verbose: boolean;
}) {
  const projectPackage = await WritableNpmPackage.fromPath(process.cwd());
  if (!projectPackage) {
    throw new Error(
      `The current directory '${process.cwd()}' is not the root of an npm package`,
    );
  }

  await projectPackage.mergeProps({
    scripts: {windows: 'react-native run-windows'},
  });

  const rnwPackage = await findPackage('react-native-windows');
  if (!rnwPackage) {
    throw new Error('Could not locate the package for react-native-windows');
  }

  const rnPackage = await findPackage('react-native');
  if (!rnPackage) {
    throw new Error('Could not locate the package for react-native');
  }

  // We add an exclusionList from metro config. This will be hoisted, but add
  // an explict dep because we require it directly.
  const cliPackage = await findPackage('@react-native-community/cli', {
    searchPath: rnPackage.path,
  });
  const metroConfigPackage = await findPackage('metro-config', {
    searchPath: cliPackage?.path || rnPackage.path,
  });

  if (metroConfigPackage) {
    await projectPackage.mergeProps({
      devDependencies: {
        'metro-config': `^${metroConfigPackage.json.version}`,
      },
    });
  }

  const rnPeerDependency = rnwPackage.json.peerDependencies['react-native'];

  if (
    !semver.satisfies(rnPackage.json.version, rnPeerDependency) &&
    projectPackage.json.dependencies?.['react-native']
  ) {
    console.log(
      chalk.green('Installing a compatible version of react-native:'),
    );
    console.log(chalk.white(`    ${rnPeerDependency}`));

    // Patch package.json to have proper react-native version and install
    await projectPackage.mergeProps({
      dependencies: {'react-native': rnPeerDependency},
    });

    // Install dependencies using correct package manager
    const isYarn = fs.existsSync(path.join(process.cwd(), 'yarn.lock'));
    childProcess.execSync(
      isYarn ? 'yarn' : 'npm i',
      options.verbose ? {stdio: 'inherit'} : {},
    );
  }
}
