/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

export interface TelemetryProject {
  id: string;
  platforms: Array<string>;
  rnwLang: 'cpp' | 'cs' | null;
}

export interface TelemetryLibProject extends TelemetryProject {}

export interface TelemetryAppProject extends TelemetryProject {
  usesTS: boolean;
  usesRNConfig: boolean;
  jsEngine: string;
  rnwSource: string;
  dependencies: Array<TelemetryLibProject>;
}
