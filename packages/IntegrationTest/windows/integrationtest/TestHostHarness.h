// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <ReactContext.h>
#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Sockets.h>

#include "TestCommandListener.h"

namespace IntegrationTest {

//! Allows controlling the ReactHost based on messages from a TestRunner
class TestHostHarness : public winrt::implements<TestHostHarness, winrt::Windows::Foundation::IInspectable> {
  friend class TestHostHarnessRedboxHandler;

 public:
  TestHostHarness() noexcept;

  void SetRootView(winrt::Microsoft::ReactNative::ReactRootView &&rootView) noexcept;
  void SetReactHost(winrt::Microsoft::ReactNative::ReactNativeHost &&reactHost) noexcept;

 private:
  void ShowJSError(std::string_view err) noexcept;
  winrt::fire_and_forget StartListening() noexcept;
  winrt::fire_and_forget OnTestCommand(TestCommand command, TestCommandResponse response) noexcept;
  void OnTestCompleted() noexcept;
  void OnTestPassed(bool passed) noexcept;
  winrt::Windows::Foundation::IAsyncAction TimeoutOnInactivty() noexcept;
  winrt::Windows::Foundation::IAsyncAction FlushJSQueue() noexcept;
  void CompletePendingResponse() noexcept;

  winrt::Microsoft::ReactNative::ReactRootView m_rootView;
  winrt::Microsoft::ReactNative::ReactContext m_context;
  winrt::Microsoft::ReactNative::IReactInstanceSettings::InstanceLoaded_revoker m_instanceLoadedRevoker;

  winrt::com_ptr<TestCommandListener> m_commandListener;
  winrt::Microsoft::ReactNative::IRedBoxHandler m_redboxHandler;
  std::optional<TestCommandResponse> m_pendingResponse;
  winrt::Windows::Foundation::IAsyncAction m_timeoutAction;
};

//! Redbox handler which feeds into the TestHostHarness to communicate exceptions to the test runner
class TestHostHarnessRedboxHandler
    : public winrt::implements<TestHostHarnessRedboxHandler, winrt::Microsoft::ReactNative::IRedBoxHandler> {
 public:
  TestHostHarnessRedboxHandler(winrt::weak_ref<TestHostHarness> &&weakHarness) noexcept
      : m_weakHarness(std::move(weakHarness)) {}

  void ShowNewError(
      const winrt::Microsoft::ReactNative::IRedBoxErrorInfo &info,
      winrt::Microsoft::ReactNative::RedBoxErrorType type) noexcept;
  bool IsDevSupportEnabled() noexcept;
  void UpdateError(const winrt::Microsoft::ReactNative::IRedBoxErrorInfo &info) noexcept;
  void DismissRedBox() noexcept;

 private:
  void HandleException() noexcept;

  winrt::weak_ref<TestHostHarness> m_weakHarness;
  std::unique_ptr<ExceptionInfo> m_pendingException;
};
} // namespace IntegrationTest
