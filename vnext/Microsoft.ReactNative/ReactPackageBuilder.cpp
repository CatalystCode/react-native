// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactPackageBuilder.h"

namespace winrt::Microsoft::ReactNative::implementation {

//===========================================================================
// ReactPackageBuilder implementation
//===========================================================================

ReactPackageBuilder::ReactPackageBuilder(
    std::shared_ptr<NativeModulesProvider> const &modulesProvider,
#ifndef CORE_ABI
    std::shared_ptr<ViewManagersProvider> const &viewManagersProvider,
#endif
    std::shared_ptr<TurboModulesProvider> const &turboModulesProvider) noexcept
    : m_modulesProvider{modulesProvider},
#ifndef CORE_ABI
      m_viewManagersProvider{viewManagersProvider},
#endif
      m_turboModulesProvider{turboModulesProvider} {
}

void ReactPackageBuilder::AddModule(hstring const &moduleName, ReactModuleProvider const &moduleProvider) noexcept {
  m_modulesProvider->AddModuleProvider(moduleName, moduleProvider, nullptr);
}

#ifndef CORE_ABI
void ReactPackageBuilder::AddViewManager(
    hstring const &viewManagerName,
    ReactViewManagerProvider const &viewManagerProvider) noexcept {
  m_viewManagersProvider->AddViewManagerProvider(viewManagerName, viewManagerProvider);
}
#endif

void ReactPackageBuilder::AddTurboModule(
    hstring const &moduleName,
    ReactModuleProvider const &moduleProvider) noexcept {
  m_turboModulesProvider->AddModuleProvider(moduleName, moduleProvider);
}

void ReactPackageBuilder::AddDispatchedModule(
    hstring const &moduleName,
    ReactModuleProvider const &moduleProvider,
    IReactPropertyName const &dispatcherName) noexcept {
  m_modulesProvider->AddModuleProvider(moduleName, moduleProvider, dispatcherName);
}

} // namespace winrt::Microsoft::ReactNative::implementation
