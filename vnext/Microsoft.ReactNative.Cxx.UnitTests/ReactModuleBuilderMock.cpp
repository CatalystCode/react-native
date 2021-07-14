// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"
#include "JSValue.h"
#include "JSValueTreeWriter.h"

namespace winrt::Microsoft::ReactNative {

//===========================================================================
// ReactModuleBuilderMock implementation
//===========================================================================

ReactModuleBuilderMock::ReactModuleBuilderMock() noexcept : m_reactContext{make<ReactContextMock>(this)} {}

void ReactModuleBuilderMock::ExpectEvent(
    std::wstring_view eventEmitterName,
    std::wstring_view eventName,
    Mso::Functor<void(JSValueArray const &)> &&checkValues) noexcept {
  m_jsEventHandler =
      [expectedEventEmitterName = std::wstring{eventEmitterName},
       expectedEventName = std::wstring{eventName},
       checkValues = std::move(checkValues)](
          std::wstring_view eventEmitterName, std::wstring_view eventName, JSValue const &value) noexcept {
        TestCheck(expectedEventEmitterName == eventEmitterName);
        TestCheck(expectedEventName == eventName);
        TestCheck(value.Type() == JSValueType::Array);
        checkValues(value.AsArray());
      };
}

void ReactModuleBuilderMock::ExpectFunction(
    std::wstring_view moduleName,
    std::wstring_view functionName,
    Mso::Functor<void(JSValueArray const &)> &&checkValues) noexcept {
  m_jsFunctionHandler = [expectedModuleName = std::wstring{moduleName},
                         expectedFuncName = std::wstring{functionName},
                         checkValues = std::move(checkValues)](
                            std::wstring_view moduleName, std::wstring_view funcName, JSValue const &value) noexcept {
    TestCheck(expectedModuleName == moduleName);
    TestCheck(expectedFuncName == funcName);
    TestCheck(value.Type() == JSValueType::Array);
    checkValues(value.AsArray());
  };
}

void ReactModuleBuilderMock::CallJSFunction(
    std::wstring_view moduleName,
    std::wstring_view functionName,
    JSValueArgWriter const &paramsArgWriter) noexcept {
  auto writer = MakeJSValueTreeWriter();
  paramsArgWriter(writer);
  m_jsFunctionHandler(moduleName, functionName, TakeJSValue(writer));
}

void ReactModuleBuilderMock::EmitJSEvent(
    std::wstring_view eventEmitterName,
    std::wstring_view eventName,
    JSValueArgWriter const &paramsArgWriter) noexcept {
  auto writer = MakeJSValueTreeWriter();
  writer.WriteArrayBegin();
  paramsArgWriter(writer);
  writer.WriteArrayEnd();
  m_jsEventHandler(eventEmitterName, eventName, TakeJSValue(writer));
}

void ReactModuleBuilderMock::AddInitializer(InitializerDelegate const &initializer) noexcept {
  m_initializers.push_back(initializer);
}

void ReactModuleBuilderMock::AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept {
  m_constantProviders.push_back(constantProvider);
}

void ReactModuleBuilderMock::AddMethod(
    hstring const &name,
    MethodReturnType returnType,
    MethodDelegate const &method) noexcept {
  m_methods.emplace(name, std::tuple<MethodReturnType, MethodDelegate>{returnType, method});
}

void ReactModuleBuilderMock::AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept {
  m_syncMethods.emplace(name, method);
}

JSValueObject ReactModuleBuilderMock::GetConstants() noexcept {
  auto constantWriter = MakeJSValueTreeWriter();
  constantWriter.WriteObjectBegin();
  for (const auto &constantProvider : m_constantProviders) {
    constantProvider(constantWriter);
  }

  constantWriter.WriteObjectEnd();
  return TakeJSValue(constantWriter).MoveObject();
}

MethodDelegate ReactModuleBuilderMock::GetMethod0(std::wstring const &methodName) const noexcept {
  auto it = m_methods.find(methodName);
  return (it != m_methods.end() && std::get<0>(it->second) == MethodReturnType::Void) ? std::get<1>(it->second)
                                                                                      : nullptr;
}

MethodDelegate ReactModuleBuilderMock::GetMethod1(std::wstring const &methodName) const noexcept {
  auto it = m_methods.find(methodName);
  return (it != m_methods.end() && std::get<0>(it->second) == MethodReturnType::Callback) ? std::get<1>(it->second)
                                                                                          : nullptr;
}

MethodDelegate ReactModuleBuilderMock::GetMethod2(std::wstring const &methodName) const noexcept {
  auto it = m_methods.find(methodName);
  return (it != m_methods.end() &&
          (std::get<0>(it->second) == MethodReturnType::TwoCallbacks ||
           std::get<0>(it->second) == MethodReturnType::Promise))
      ? std::get<1>(it->second)
      : nullptr;
}

SyncMethodDelegate ReactModuleBuilderMock::GetSyncMethod(std::wstring const &methodName) const noexcept {
  auto it = m_syncMethods.find(methodName);
  return (it != m_syncMethods.end()) ? it->second : nullptr;
}

/*static*/ IJSValueWriter ReactModuleBuilderMock::ArgWriter() noexcept {
  return MakeJSValueTreeWriter();
}

/*static*/ IJSValueReader ReactModuleBuilderMock::CreateArgReader(
    std::function<void(IJSValueWriter const &)> const &argWriter) noexcept {
  auto writer = MakeJSValueTreeWriter();
  argWriter(writer);
  return MakeJSValueTreeReader(TakeJSValue(writer));
}

Windows::Foundation::IInspectable ReactModuleBuilderMock::CreateModule(
    ReactModuleProvider const &provider,
    IReactModuleBuilder const &moduleBuilder) noexcept {
  auto result = provider(moduleBuilder);
  for (auto &initializer : m_initializers) {
    initializer(m_reactContext);
  }

  return result;
}

ReactContextMock::ReactContextMock(ReactModuleBuilderMock *builderMock) noexcept : m_builderMock{builderMock} {}

void ReactContextMock::CallJSFunction(
    hstring const &moduleName,
    hstring const &functionName,
    JSValueArgWriter const &paramsArgWriter) noexcept {
  m_builderMock->CallJSFunction(moduleName, functionName, paramsArgWriter);
}

void ReactContextMock::EmitJSEvent(
    hstring const &eventEmitterName,
    hstring const &eventName,
    JSValueArgWriter const &paramsArgWriter) noexcept {
  m_builderMock->EmitJSEvent(eventEmitterName, eventName, paramsArgWriter);
}

Microsoft::ReactNative::IReactPropertyNamespace ReactPropertyBagHelper::GlobalNamespace() {
  VerifyElseCrashSz(false, "Not implemented");
}

Microsoft::ReactNative::IReactPropertyNamespace ReactPropertyBagHelper::GetNamespace(
    param::hstring const & /*namespaceName*/) {
  VerifyElseCrashSz(false, "Not implemented");
}

Microsoft::ReactNative::IReactPropertyName ReactPropertyBagHelper::GetName(
    Microsoft::ReactNative::IReactPropertyNamespace const & /*ns*/,
    param::hstring const & /*localName*/) {
  VerifyElseCrashSz(false, "Not implemented");
}

Microsoft::ReactNative::IReactPropertyBag ReactPropertyBagHelper::CreatePropertyBag() {
  VerifyElseCrashSz(false, "Not implemented");
}

IReactPropertyName ReactDispatcherHelper::UIDispatcherProperty() {
  return nullptr;
}

IReactPropertyName ReactDispatcherHelper::JSDispatcherProperty() {
  return nullptr;
}

} // namespace winrt::Microsoft::ReactNative
