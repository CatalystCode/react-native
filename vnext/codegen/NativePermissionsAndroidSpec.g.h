
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

struct PermissionsAndroidSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(std::string,Promise<React::JSValue>) noexcept>{0, L"checkPermission"},
      Method<void(std::string,Promise<React::JSValue>) noexcept>{1, L"requestPermission"},
      Method<void(std::string,Promise<React::JSValue>) noexcept>{2, L"shouldShowRequestPermissionRationale"},
      Method<void(JSValueArray,Promise<React::JSValue>) noexcept>{3, L"requestMultiplePermissions"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, PermissionsAndroidSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
          0,
          "checkPermission",
          "    REACT_METHOD(checkPermission) void checkPermission(std::string permission, React::ReactPromise<React::JSValue> &&result) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          1,
          "requestPermission",
          "    REACT_METHOD(requestPermission) void requestPermission(std::string permission, React::ReactPromise<React::JSValue> &&result) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          2,
          "shouldShowRequestPermissionRationale",
          "    REACT_METHOD(shouldShowRequestPermissionRationale) void shouldShowRequestPermissionRationale(std::string permission, React::ReactPromise<React::JSValue> &&result) noexcept { /* implementation */ }}");
    REACT_SHOW_METHOD_SPEC_ERRORS(
          3,
          "requestMultiplePermissions",
          "    REACT_METHOD(requestMultiplePermissions) void requestMultiplePermissions(React::JSValueArray && permissions, React::ReactPromise<React::JSValue> &&result) noexcept { /* implementation */ }}");
  }
};

} // namespace Microsoft::ReactNativeSpecs
