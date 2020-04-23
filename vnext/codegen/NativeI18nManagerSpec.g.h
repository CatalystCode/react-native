
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

struct I18nManagerSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(bool) noexcept>{0, L"allowRTL"},
      Method<void(bool) noexcept>{1, L"forceRTL"},
      Method<void(bool) noexcept>{2, L"swapLeftAndRightInRTL"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, I18nManagerSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "allowRTL",
          "    REACT_METHOD(allowRTL) void allowRTL(bool allowRTL) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "forceRTL",
          "    REACT_METHOD(forceRTL) void forceRTL(bool forceRTL) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "swapLeftAndRightInRTL",
          "    REACT_METHOD(swapLeftAndRightInRTL) void swapLeftAndRightInRTL(bool flipStyles) noexcept { /* implementation */ }}");
  }
};

} // namespace Microsoft::ReactNativeSpecs
