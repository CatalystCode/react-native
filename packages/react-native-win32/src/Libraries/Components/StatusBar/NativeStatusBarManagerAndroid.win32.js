/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

'use strict';

import type {TurboModule} from '../../TurboModule/RCTExport';
import * as TurboModuleRegistry from '../../TurboModule/TurboModuleRegistry';
import type {NativeOrDynamicColorType} from '../../StyleSheet/NativeOrDynamicColorType'; // [Win32]

export interface Spec extends TurboModule {
  +getConstants: () => {|
    +HEIGHT: number,
    +DEFAULT_BACKGROUND_COLOR: number,
  |};
  // [Win32 Allow NativeOrDynamicColorType to make Flow happy (this will never be called)
  +setColor: (
    color: number | NativeOrDynamicColorType,
    animated: boolean,
  ) => void;
  // Win32]
  +setTranslucent: (translucent: boolean) => void;

  /**
   *  - statusBarStyles can be:
   *    - 'default'
   *    - 'dark-content'
   */
  +setStyle: (statusBarStyle?: ?string) => void;
  +setHidden: (hidden: boolean) => void;
}

// [Win32 Change from getEnforcing to get and provide a stub (See #4363). This
// can be removed once devmain integrates with 0.62 native bits.
let NativeStausBarManager = TurboModuleRegistry.get<Spec>('StatusBarManager');
if (!NativeStausBarManager) {
  NativeStausBarManager = {
    getConstants: () => ({HEIGHT: 0, DEFAULT_BACKGROUND_COLOR: 0}),
    setColor: (
      color: number | NativeOrDynamicColorType,
      animated: boolean,
    ) => {},
    setTranslucent: (translucent: boolean) => {},
    setStyle: (statusBarStyle?: ?string) => {},
    setHidden: (hidden: boolean) => {},
  };
}

export default (NativeStausBarManager: Spec);
// Win32]
