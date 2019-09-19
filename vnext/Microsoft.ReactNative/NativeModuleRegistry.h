#pragma once
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Bridge.NativeModuleRegistry.g.h"

using namespace winrt;
using namespace Windows::Foundation::Collections;

namespace winrt::Microsoft::ReactNative::Bridge::implementation {

struct NativeModuleRegistry : NativeModuleRegistryT<NativeModuleRegistry> {
  NativeModuleRegistry() = default;
  NativeModuleRegistry(IVectorView<Bridge::INativeModule> const &modules)
      : m_modules(modules) {
    if (modules == nullptr || modules.Size() == 0) {
      throw hresult_invalid_argument(L"List of modules is empty.");
    }
  }

  IVectorView<Bridge::INativeModule> Modules() {
    return m_modules;
  }

 private:
  IVectorView<Bridge::INativeModule> m_modules{nullptr};
};

} // namespace winrt::Microsoft::ReactNative::Bridge::implementation

namespace winrt::Microsoft::ReactNative::Bridge::factory_implementation {

struct NativeModuleRegistry : NativeModuleRegistryT<
                                  NativeModuleRegistry,
                                  implementation::NativeModuleRegistry> {};

} // namespace winrt::Microsoft::ReactNative::Bridge::factory_implementation
