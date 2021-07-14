// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include <ReactNonAbiValue.h>
#include <Shared/IRedBoxHandler.h>
#include <unicode.h>
#include "DynamicReader.h"
#include "DynamicWriter.h"
#include "IReactContext.h"
#include "IReactModuleBuilder.h"
#include "ReactPackageBuilder.h"
#include "RedBoxErrorFrameInfo.h"
#include "RedBoxErrorInfo.h"
#include "TestController.h"

#include <ReactHost/ReactContext.h>

#include "Microsoft.Internal.TestController.g.cpp"

using namespace winrt;
namespace msrn = ::winrt::Microsoft::ReactNative; // helps avoid collisions with 'Microsoft' namespace in unicode.h

namespace {

// Extends the 'DynamicReader' class with the ability the manage the lifetime of the folly::dynamic value it's reading
// from.
struct DynamicWrapperReader : public winrt::implements<DynamicWrapperReader, msrn::IJSValueReader> {
  DynamicWrapperReader(folly::dynamic &&value) : m_value{std::move(value)} {
    m_innerReader = make<msrn::DynamicReader>(m_value);
  }

  msrn::JSValueType ValueType() noexcept {
    return m_innerReader.ValueType();
  }

  bool GetNextObjectProperty(hstring &propertyName) noexcept {
    return m_innerReader.GetNextObjectProperty(propertyName);
  }

  bool GetNextArrayItem() noexcept {
    return m_innerReader.GetNextArrayItem();
  }

  hstring GetString() noexcept {
    return m_innerReader.GetString();
  }

  bool GetBoolean() noexcept {
    return m_innerReader.GetBoolean();
  }

  int64_t GetInt64() noexcept {
    return m_innerReader.GetInt64();
  }

  double GetDouble() noexcept {
    return m_innerReader.GetDouble();
  }

 private:
  msrn::IJSValueReader m_innerReader;
  folly::dynamic m_value;
};

} // namespace

namespace winrt::Microsoft::Internal::implementation {

msrn::IJSValueReader TestController::CreateDynamicReader(msrn::IJSValueWriter writer) {
  auto dw = writer.as<msrn::DynamicWriter>();
  return make<DynamicWrapperReader>(dw->TakeValue());
}

msrn::IJSValueWriter TestController::CreateDynamicWriter() {
  return make<msrn::DynamicWriter>();
}

msrn::IReactContext TestController::CreateContext(
    const msrn::IReactPropertyBag &propertyBag,
    const msrn::IReactNotificationService &notificationService) {
  auto innerContext = Mso::Make<Mso::React::ReactContext>(nullptr, propertyBag, notificationService);
  return make<msrn::implementation::ReactContext>(std::move(innerContext));
}

msrn::IReactModuleBuilder TestController::CreateReactModuleBuilder(msrn::IReactContext context) {
  return make<msrn::ReactModuleBuilder>(context);
}

msrn::IReactPackageBuilder TestController::CreateReactPackageBuilder() {
  auto nativeModulesProvider = std::make_shared<msrn::NativeModulesProvider>();
  auto turboModulesProvider = std::make_shared<msrn::TurboModulesProvider>();
  return make<msrn::implementation::ReactPackageBuilder>(nativeModulesProvider, turboModulesProvider);
}

msrn::IRedBoxErrorFrameInfo
TestController::CreateRedBoxErrorFrameInfo(hstring file, hstring method, uint32_t line, uint32_t column) {
  Mso::React::ErrorFrameInfo frameInfo{
      /* File */ ::Microsoft::Common::Unicode::Utf16ToUtf8(file),
      /* Method */ ::Microsoft::Common::Unicode::Utf16ToUtf8(method),
      /* Line */ static_cast<int>(line),
      /* Columns */ static_cast<int>(column),
      /* Collapse */ false};

  return make<Mso::React::RedBoxErrorFrameInfo>(std::move(frameInfo));
}

msrn::IRedBoxErrorInfo TestController::CreateRedBoxErrorInfo(
    hstring message,
    uint32_t id,
    array_view<Microsoft::ReactNative::IRedBoxErrorFrameInfo const> callstack) {
  // The repeated value conversion between CreateRedBoxErrorFrameInfo and CreateRedBoxErrorInfo is suboptimal, but
  // deemed acceptable because we only need this functionality for ABI testing.

  Mso::React::ErrorInfo errorInfo;
  errorInfo.Message = ::Microsoft::Common::Unicode::Utf16ToUtf8(message);
  errorInfo.Id = id;

  // leaving the following ErrorInfo members out to avoid an excessive parameter list and because they probably add
  // little test value
  //
  // errorInfo.OriginalMessage
  // errorInfo.Name
  // errorInfo.ComponentStack
  // errorInfo.ExtraData

  for (auto const &frame : callstack) {
    Mso::React::ErrorFrameInfo frameInfo{
        /* File */ ::Microsoft::Common::Unicode::Utf16ToUtf8(frame.File()),
        /* Method */ ::Microsoft::Common::Unicode::Utf16ToUtf8(frame.Method()),
        /* Line */ static_cast<int>(frame.Line()),
        /* Columns */ static_cast<int>(frame.Column()),
        /* Collapse */ false};
    errorInfo.Callstack.push_back(std::move(frameInfo));
  }

  return make<Mso::React::RedBoxErrorInfo>(std::move(errorInfo));
}

msrn::IReactNonAbiValue TestController::CreateNonAbiValue(int32_t value) {
  return make<msrn::implementation::ReactNonAbiValue<int32_t>>(value);
}

} // namespace winrt::Microsoft::Internal::implementation
