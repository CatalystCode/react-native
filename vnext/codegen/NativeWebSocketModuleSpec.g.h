
/*
 * This file is auto-generated from a NativeModule spec file in js.
 *
 * This is a C++ Spec class that should be used with MakeTurboModuleProvider to register native modules
 * in a way that also verifies at compile time that the native module matches the interface required
 * by the TurboModule JS spec.
 */
#pragma once

#include "NativeModules.h"
#include <tuple>

namespace Microsoft::ReactNativeSpecs {

struct WebSocketModuleSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(std::string,JSValueArray,JSValueObject,double) noexcept>{0, L"connect"},
      Method<void(std::string,double) noexcept>{1, L"send"},
      Method<void(std::string,double) noexcept>{2, L"sendBinary"},
      Method<void(double) noexcept>{3, L"ping"},
      Method<void(double,std::string,double) noexcept>{4, L"close"},
      Method<void(std::string) noexcept>{5, L"addListener"},
      Method<void(double) noexcept>{6, L"removeListeners"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, WebSocketModuleSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "connect",
          "    REACT_METHOD(connect) void connect(std::string url, React::JSValueArray && protocols, React::JSValueObject && options, double socketID) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "send",
          "    REACT_METHOD(send) void send(std::string message, double forSocketID) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "sendBinary",
          "    REACT_METHOD(sendBinary) void sendBinary(std::string base64String, double forSocketID) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          3,
          "ping",
          "    REACT_METHOD(ping) void ping(double socketID) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          4,
          "close",
          "    REACT_METHOD(close) void close(double code, std::string reason, double socketID) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          5,
          "addListener",
          "    REACT_METHOD(addListener) void addListener(std::string eventName) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          6,
          "removeListeners",
          "    REACT_METHOD(removeListeners) void removeListeners(double count) noexcept { /* implementation */ }}");
  }
};

} // namespace Microsoft::ReactNativeSpecs
