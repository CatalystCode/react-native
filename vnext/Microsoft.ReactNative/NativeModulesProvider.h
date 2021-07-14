// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <NativeModuleProvider.h>
#include <winrt/Microsoft.ReactNative.h>
#include "ReactHost/React.h"

namespace winrt::Microsoft::ReactNative {

class NativeModulesProvider final : public Mso::React::NativeModuleProvider2 {
 public:
  virtual std::vector<facebook::react::NativeModuleDescription> GetModules(
      Mso::CntPtr<Mso::React::IReactContext> const &reactContext,
      std::shared_ptr<facebook::react::MessageQueueThread> const &defaultQueueThread) override;

 public:
  void AddModuleProvider(
      winrt::hstring const &moduleName,
      ReactModuleProvider const &moduleProvider,
      IReactPropertyName const &dispatcherName) noexcept;

 private:
  std::map<std::string, std::pair<ReactModuleProvider, IReactPropertyName>> m_moduleProviders;
  IReactPackageBuilder m_packageBuilder;
};

} // namespace winrt::Microsoft::ReactNative
