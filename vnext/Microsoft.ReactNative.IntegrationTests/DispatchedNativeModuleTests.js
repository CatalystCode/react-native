// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import { NativeModules } from 'react-native';

const {
  TestDispatchedModule1,
  TestDispatchedModule2,
  TestDispatchedModule3,
  TestDispatchedModule4,
} = NativeModules;

TestDispatchedModule1.testCheckDispatcher();
TestDispatchedModule2.testCheckDispatcher();
TestDispatchedModule3.testCheckDispatcher();
TestDispatchedModule4.testCheckDispatcher();
