/**
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * @format
 */

import {botFunction} from '../botFunction';

export default botFunction(async ({actorsHandle}) => {
  await actorsHandle.emitEvent('integration-timer-fired');
});
