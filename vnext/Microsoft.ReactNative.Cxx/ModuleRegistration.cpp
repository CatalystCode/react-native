// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// IMPORTANT: Before updating this file
// please read react-native-windows repo:
// vnext/Microsoft.ReactNative.Cxx/README.md

#include "pch.h"
#include "ModuleRegistration.h"

namespace winrt::Microsoft::ReactNative {

const ReactModuleRegistration *ReactModuleRegistration::s_head{nullptr};

ReactModuleRegistration::ReactModuleRegistration(ReactModuleInfo &&moduleInfo) noexcept
    : m_moduleInfo{std::move(moduleInfo)}, m_next{s_head} {
  s_head = this;
}

void AddAttributedModules(ReactPackageBuilder const &packageBuilder) noexcept {
  for (auto const *reg = ReactModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
    packageBuilder.AddDispatchedModule(
        reg->ModuleInfo().ModuleName, reg->MakeModuleProvider(), reg->ModuleInfo().DispatcherName);
  }
}

bool TryAddAttributedModule(ReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept {
  for (auto const *reg = ReactModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
    if (moduleName == reg->ModuleInfo().ModuleName) {
      packageBuilder.AddDispatchedModule(moduleName, reg->MakeModuleProvider(), reg->ModuleInfo().DispatcherName);
      return true;
    }
  }
  return false;
}

void AddAttributedModules(IReactPackageBuilder const &packageBuilder) noexcept {
  AddAttributedModules(packageBuilder.as<ReactPackageBuilder>());
}

bool TryAddAttributedModule(IReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept {
  return TryAddAttributedModule(packageBuilder.as<ReactPackageBuilder>(), moduleName);
}

} // namespace winrt::Microsoft::ReactNative
