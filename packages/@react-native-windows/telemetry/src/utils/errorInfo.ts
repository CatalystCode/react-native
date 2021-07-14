/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 * @format
 */

import {CodedErrorType} from '../CodedError';

export interface TelemetryCodedError {
  type: CodedErrorType;
  rawErrorCode: string | null;
  data: Record<string, any> | null;
}
