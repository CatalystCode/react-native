/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

export const CodedErrors = {
  Success: 0,
  // init
  UnsupportedReactNativeVersion: 1,
  UserCancel: 2,
  NoReactNativeFound: 3,
  NoPackageJSon: 4,
  NoLatestReactNativeWindows: 5,
  NoAutoMatchingReactNativeWindows: 6,
  IncompatibleOptions: 7,
  NoReactNativeDependencies: 8,
  NoMatchingPackageVersion: 9,
  GeneratorNotFound: 10,
  // run-windows
  NoSolution: 100,
  // Project generation
  NoPropertyInProject: 200,
  CopyProjectTemplateNoSourcePath: 201,
  CopyProjectTemplateNoDestPath: 202,
  CopyProjectTemplateNoProjectName: 203,
  // Config and Autolinking
  NoWindowsConfig: 300,
  IncompleteConfig: 301,
  InvalidConfig: 302,
  NeedAutolinking: 303,
  AddProjectToSolution: 304,
  Autolinking: 305,
  // SDK requirements
  MinSDKVersionNotMet: 400,
  BadSDKVersionFormat: 401,
  NoSDK: 402,
  // Build
  NoMSBuild: 500,
  NoVSWhere: 501,
  MSBuildError: 502,
  // Deploy
  NoAppPackage: 600,
  NoAppxManifest: 601,
  NoDevice: 602,
  AppDidNotDeploy: 603,
  InvalidDevicesOutput: 604,
  RemoveOldAppVersionFailure: 605,
  EnableDevModeFailure: 606,
  InstallAppFailure: 607,
  InstallAppDependenciesFailure: 608,
  CheckNetIsolationFailure: 709,
  InstallAppToDeviceFailure: 710,
  UninstallAppOnDeviceFailure: 711,
  DeployRecipeFailure: 712,

  // Others
  AppStartupFailure: 700,

  //
  Unknown: -1,
};

export type CodedErrorType = keyof typeof CodedErrors;

/**
 * Represents an error whose message might contain user-originating content,
 * therefore when transmitting telemetry, only the type should be sent.
 * @param type a stable ID identifying the type of error.
 * @param message the error text. This should only be used for display to the user.
 * @param data any additional metadata that is safe to collect for telemetry purposes.
 */
export class CodedError extends Error {
  constructor(
    type: CodedErrorType,
    message: string,
    public readonly data?: Record<string, any>,
  ) {
    super(message);
    this.name = type;
  }
}
