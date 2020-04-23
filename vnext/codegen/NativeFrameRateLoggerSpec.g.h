
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

struct FrameRateLoggerSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(JSValueObject) noexcept>{0, L"setGlobalOptions"},
      Method<void(std::string) noexcept>{1, L"setContext"},
      Method<void() noexcept>{2, L"beginScroll"},
      Method<void() noexcept>{3, L"endScroll"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, FrameRateLoggerSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "setGlobalOptions",
          "    REACT_METHOD(setGlobalOptions) void setGlobalOptions(React::JSValueObject && options) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "setContext",
          "    REACT_METHOD(setContext) void setContext(std::string context) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "beginScroll",
          "    REACT_METHOD(beginScroll) void beginScroll() noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          3,
          "endScroll",
          "    REACT_METHOD(endScroll) void endScroll() noexcept { /* implementation */ }}");
  }
};

} // namespace Microsoft::ReactNativeSpecs
