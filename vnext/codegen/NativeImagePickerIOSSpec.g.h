
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

struct ImagePickerIOSSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(Callback<JSValue>) noexcept>{0, L"canRecordVideos"},
      Method<void(Callback<JSValue>) noexcept>{1, L"canUseCamera"},
      Method<void(JSValueObject,Callback<JSValue>,Callback<JSValue>) noexcept>{2, L"openCameraDialog"},
      Method<void(JSValueObject,Callback<JSValue>,Callback<JSValue>) noexcept>{3, L"openSelectDialog"},
      Method<void() noexcept>{4, L"clearAllPendingVideos"},
      Method<void(std::string) noexcept>{5, L"removePendingVideo"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, ImagePickerIOSSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "canRecordVideos",
          "    REACT_METHOD(canRecordVideos) void canRecordVideos(std::function<void(React::JSValue const &)> const & callback) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "canUseCamera",
          "    REACT_METHOD(canUseCamera) void canUseCamera(std::function<void(React::JSValue const &)> const & callback) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "openCameraDialog",
          "    REACT_METHOD(openCameraDialog) void openCameraDialog(React::JSValueObject && config, std::function<void(React::JSValue const &)> const & successCallback, std::function<void(React::JSValue const &)> const & cancelCallback) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          3,
          "openSelectDialog",
          "    REACT_METHOD(openSelectDialog) void openSelectDialog(React::JSValueObject && config, std::function<void(React::JSValue const &)> const & successCallback, std::function<void(React::JSValue const &)> const & cancelCallback) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          4,
          "clearAllPendingVideos",
          "    REACT_METHOD(clearAllPendingVideos) void clearAllPendingVideos() noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          5,
          "removePendingVideo",
          "    REACT_METHOD(removePendingVideo) void removePendingVideo(std::string url) noexcept { /* implementation */ }}");
  }
};

} // namespace Microsoft::ReactNativeSpecs
