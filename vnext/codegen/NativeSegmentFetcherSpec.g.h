
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

struct SegmentFetcherSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(double,JSValueObject,Callback<JSValue>) noexcept>{0, L"fetchSegment"},
      Method<void(double,JSValueObject,Callback<JSValue>) noexcept>{1, L"getSegment"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, SegmentFetcherSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "fetchSegment",
          "    REACT_METHOD(fetchSegment) void fetchSegment(double segmentId, React::JSValueObject && options, std::function<void(React::JSValue const &)> const & callback) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "getSegment",
          "    REACT_METHOD(getSegment) void getSegment(double segmentId, React::JSValueObject && options, std::function<void(React::JSValue const &)> const & callback) noexcept { /* implementation */ }}");
  }
};

} // namespace Microsoft::ReactNativeSpecs
