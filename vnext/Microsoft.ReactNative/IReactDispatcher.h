// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "ReactDispatcherHelper.g.h"
#include <Threading/MessageDispatchQueue.h>
#include <dispatchQueue/dispatchQueue.h>
#include <winrt/Microsoft.ReactNative.h>

namespace winrt::Microsoft::ReactNative::implementation {

struct ReactDispatcher : implements<ReactDispatcher, default_interface<IReactDispatcher>> {
  ReactDispatcher() = default;
  ReactDispatcher(Mso::DispatchQueue &&queue) noexcept;

  bool HasThreadAccess() noexcept;
  void Post(ReactDispatcherCallback const &callback) noexcept;

  std::shared_ptr<facebook::react::MessageQueueThread> GetMessageQueueThread() const noexcept;

  static IReactDispatcher CreateSerialDispatcher() noexcept;

  static Mso::DispatchQueue GetUIDispatchQueue(IReactPropertyBag const &properties) noexcept;

  static IReactDispatcher UIThreadDispatcher() noexcept;
  static IReactPropertyName UIDispatcherProperty() noexcept;
  static IReactDispatcher GetUIDispatcher(IReactPropertyBag const &properties) noexcept;
  static void SetUIThreadDispatcher(IReactPropertyBag const &properties) noexcept;

  static IReactPropertyName JSDispatcherProperty() noexcept;

 private:
  Mso::DispatchQueue m_queue;
  std::shared_ptr<Mso::React::MessageDispatchQueue> m_messageQueue;
};

struct ReactDispatcherHelper {
  ReactDispatcherHelper() = default;

  static IReactDispatcher CreateSerialDispatcher() noexcept {
    return ReactDispatcher::CreateSerialDispatcher();
  }

  static IReactDispatcher UIThreadDispatcher() noexcept {
    return ReactDispatcher::UIThreadDispatcher();
  }

  static IReactPropertyName UIDispatcherProperty() noexcept {
    return ReactDispatcher::UIDispatcherProperty();
  }

  static IReactPropertyName JSDispatcherProperty() noexcept {
    return ReactDispatcher::JSDispatcherProperty();
  }
};

} // namespace winrt::Microsoft::ReactNative::implementation

namespace winrt::Microsoft::ReactNative::factory_implementation {

struct ReactDispatcherHelper : ReactDispatcherHelperT<ReactDispatcherHelper, implementation::ReactDispatcherHelper> {};

} // namespace winrt::Microsoft::ReactNative::factory_implementation
