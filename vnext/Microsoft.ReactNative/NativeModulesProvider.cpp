// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "NativeModulesProvider.h"
#include "ABICxxModule.h"
#include "IReactContext.h"
#include "IReactDispatcher.h"
#include "IReactModuleBuilder.h"
#include "Threading/MessageQueueThreadFactory.h"

#include <folly/json.h>

#include "ReactHost/MsoUtils.h"

using namespace winrt;
using namespace Windows::Foundation;

namespace winrt::Microsoft::ReactNative {

static std::shared_ptr<facebook::react::MessageQueueThread> GetMessageQueueThread(
    Mso::React::IReactContext const &reactContext,
    IReactPropertyName const &dispatcherName,
    std::shared_ptr<facebook::react::MessageQueueThread> const &defaultQueueThread) noexcept {
  if (!dispatcherName || dispatcherName == ReactDispatcherHelper::JSDispatcherProperty()) {
    return defaultQueueThread;
  }

  return reactContext.Properties().Get(dispatcherName).as<implementation::ReactDispatcher>()->GetMessageQueueThread();
}

std::vector<facebook::react::NativeModuleDescription> NativeModulesProvider::GetModules(
    Mso::CntPtr<Mso::React::IReactContext> const &reactContext,
    std::shared_ptr<facebook::react::MessageQueueThread> const &defaultQueueThread) {
  std::vector<facebook::react::NativeModuleDescription> modules;

  auto winrtReactContext = winrt::make<implementation::ReactContext>(Mso::Copy(reactContext));

  for (auto &entry : m_moduleProviders) {
    auto messageQueueThread = GetMessageQueueThread(*reactContext, entry.second.second, defaultQueueThread);
    modules.emplace_back(
        entry.first,
        [moduleName = entry.first, moduleProvider = entry.second.first, winrtReactContext]() noexcept {
          IReactModuleBuilder moduleBuilder = winrt::make<ReactModuleBuilder>(winrtReactContext);
          auto nativeModule = moduleProvider(moduleBuilder);
          return moduleBuilder.as<ReactModuleBuilder>()->MakeCxxModule(moduleName, nativeModule);
        },
        messageQueueThread);
  }

  return modules;
}

void NativeModulesProvider::AddModuleProvider(
    winrt::hstring const &moduleName,
    ReactModuleProvider const &moduleProvider,
    IReactPropertyName const &dispatcherName) noexcept {
  m_moduleProviders.emplace(to_string(moduleName), std::make_pair(moduleProvider, dispatcherName));
}

} // namespace winrt::Microsoft::ReactNative
