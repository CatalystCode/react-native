// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// IMPORTANT: Before updating this file
// please read react-native-windows repo:
// vnext/Microsoft.ReactNative.Cxx/README.md

#pragma once
#include <string_view>
#include <tuple>
#include "winrt/Microsoft.ReactNative.h"

// We implement optional parameter macros based on the StackOverflow discussion:
// https://stackoverflow.com/questions/3046889/optional-parameters-with-c-macros
// Please refer to it if you want to understand it works.
#define INTERNAL_REACT_GET_ARG_4(arg1, arg2, arg3, arg4, ...) arg4
#define INTERNAL_REACT_RECOMPOSER_4(argsWithParentheses) INTERNAL_REACT_GET_ARG_4 argsWithParentheses

//
// The macros below are internal implementation details for macro defined in NativeModules.h
//

// Register struct as a ReactNative module.
#define INTERNAL_REACT_MODULE(moduleStruct, ...)                                                          \
  struct moduleStruct;                                                                                    \
                                                                                                          \
  /* Module registration is a template to allow static singleton definition in the header file. */        \
  template <class TDummy>                                                                                 \
  struct moduleStruct##_Registration final : winrt::Microsoft::ReactNative::ReactModuleRegistration {     \
    moduleStruct##_Registration() noexcept                                                                \
        : winrt::Microsoft::ReactNative::ReactModuleRegistration(GetModuleInfo()) {}                      \
                                                                                                          \
    /* Read optional named attributes. */                                                                 \
    static winrt::Microsoft::ReactNative::ReactModuleInfo GetModuleInfo() noexcept {                      \
      using namespace winrt::Microsoft::ReactNative;                                                      \
      /* Aliases for the most common dispatcher property names. */                                        \
      [[maybe_unused]] auto UIDispatcher = ReactDispatcherHelper::UIDispatcherProperty();                 \
      [[maybe_unused]] auto JSDispatcher = ReactDispatcherHelper::JSDispatcherProperty();                 \
      /* ReactModuleInfo default values. */                                                               \
      auto info = ReactModuleInfo{L## #moduleStruct, L"", JSDispatcher};                                  \
      /* Use lambda arguments as names for named macro arguments. */                                      \
      [&]([[maybe_unused]] ReactNamedArgs::Arg<std::wstring_view> moduleName,                             \
          [[maybe_unused]] ReactNamedArgs::Arg<std::wstring_view> eventEmitterName,                       \
          [[maybe_unused]] ReactNamedArgs::Arg<IReactPropertyName> dispatcherName) {                      \
        /* The expanded variadic macro arguments will appear in the argTuple */                           \
        /* either at predefined position or wrapped into the ReactNamedArg template. */                   \
        auto argTuple = std::make_tuple(__VA_ARGS__);                                                     \
        ReactNamedArgs::TryGet<0>(argTuple, info.ModuleName);                                             \
        ReactNamedArgs::TryGet<1>(argTuple, info.EventEmitterName);                                       \
        ReactNamedArgs::TryGet<2>(argTuple, info.DispatcherName);                                         \
      }(info.ModuleName, info.EventEmitterName, info.DispatcherName);                                     \
      return info;                                                                                        \
    }                                                                                                     \
                                                                                                          \
    winrt::Microsoft::ReactNative::ReactModuleProvider MakeModuleProvider() const noexcept override {     \
      return winrt::Microsoft::ReactNative::MakeModuleProvider<moduleStruct>();                           \
    }                                                                                                     \
                                                                                                          \
    static const moduleStruct##_Registration Registration;                                                \
  };                                                                                                      \
                                                                                                          \
  template <class TDummy>                                                                                 \
  const moduleStruct##_Registration<TDummy> moduleStruct##_Registration<TDummy>::Registration;            \
                                                                                                          \
  /* Instantiate the module registration type. */                                                         \
  template struct moduleStruct##_Registration<int>;                                                       \
                                                                                                          \
  inline winrt::Microsoft::ReactNative::ReactModuleInfo const &GetReactModuleInfo(                        \
      moduleStruct * /*valueAsTypeTag*/) noexcept {                                                       \
    return moduleStruct##_Registration<int>::Registration.ModuleInfo();                                   \
  }                                                                                                       \
                                                                                                          \
  template <class TVisitor>                                                                               \
  constexpr void VisitReactModuleMembers(moduleStruct * /*valueAsTypeTag*/, TVisitor &visitor) noexcept { \
    visitor.VisitModuleMembers(winrt::Microsoft::ReactNative::ReactAttributeId<__COUNTER__>{});           \
  }

// Provide meta data information about struct member.
// For each member with a 'custom attribute' macro we create a static method to provide meta data.
// The member Id is generated as a ReactMemberId<__COUNTER__> type.
// To enumerate the static methods, we can increment ReactMemberId while static member exists.
#define INTERNAL_REACT_MEMBER_4_ARGS(memberKind, member, jsMemberName, jsModuleName)                          \
  template <class TStruct, class TVisitor>                                                                    \
  constexpr static void GetReactMemberAttribute(                                                              \
      TVisitor &visitor, winrt::Microsoft::ReactNative::ReactAttributeId<__COUNTER__> attributeId) noexcept { \
    visitor.Visit(                                                                                            \
        &TStruct::member,                                                                                     \
        attributeId,                                                                                          \
        winrt::Microsoft::ReactNative::React##memberKind##Attribute{jsMemberName, jsModuleName});             \
  }

#define INTERNAL_REACT_MEMBER_3_ARGS(memberKind, member, jsMemberName) \
  INTERNAL_REACT_MEMBER_4_ARGS(memberKind, member, jsMemberName, L"")

#define INTERNAL_REACT_MEMBER_2_ARGS(memberKind, member) INTERNAL_REACT_MEMBER_3_ARGS(memberKind, member, L## #member)

#define INTERNAL_REACT_MEMBER(...) \
  INTERNAL_REACT_RECOMPOSER_4(     \
      (__VA_ARGS__, INTERNAL_REACT_MEMBER_4_ARGS, INTERNAL_REACT_MEMBER_3_ARGS, INTERNAL_REACT_MEMBER_2_ARGS, ))

namespace winrt::Microsoft::ReactNative {

// Support optional named arguments for the REACT_MODULE macro-attribute.
struct ReactNamedArgs {
  template <typename T>
  struct Arg {
    Arg(T &value) : m_value(value) {}

    Arg<T> &operator=(T value) noexcept {
      m_value = std::move(value);
      return *this;
    }

   private:
    T &m_value;
  };

  template <typename T>
  struct IsNamedArg : std::false_type {};

  template <typename T>
  struct IsNamedArg<Arg<T>> : std::true_type {};

  template <size_t Index, typename TArgTuple>
  static constexpr bool IsPositionalArg() {
    if constexpr (Index >= std::tuple_size_v<TArgTuple>) {
      return false;
    } else if constexpr (IsNamedArg<std::tuple_element_t<Index, TArgTuple>>::value) {
      return false;
    } else if constexpr (Index > 0) {
      static_assert(IsPositionalArg<Index - 1, TArgTuple>(), "Positional arg must not follow a named arg.");
    }
    return true;
  }

  template <size_t Index, typename TArgTuple, typename TValue>
  static void TryGet(TArgTuple &argTuple, TValue &result) {
    if constexpr (IsPositionalArg<Index, TArgTuple>()) {
      result = std::get<Index>(std::move(argTuple));
    }
  }
};

// The module information required for its registration.
struct ReactModuleInfo {
  std::wstring_view ModuleName;
  std::wstring_view EventEmitterName;
  IReactPropertyName DispatcherName;

  ReactModuleInfo(
      std::wstring_view moduleName,
      std::wstring_view eventEmitterName = L"",
      IReactPropertyName const &dispatcherName = nullptr) noexcept
      : ModuleName{moduleName}, EventEmitterName{eventEmitterName}, DispatcherName{dispatcherName} {
    if (EventEmitterName.empty()) {
      EventEmitterName = L"RCTDeviceEventEmitter";
    }
  }
};

// The base abstract class for module registration.
struct ReactModuleRegistration {
  ReactModuleRegistration(ReactModuleInfo &&moduleInfo) noexcept;

  virtual ReactModuleProvider MakeModuleProvider() const noexcept = 0;

  static ReactModuleRegistration const *Head() noexcept {
    return s_head;
  }

  ReactModuleRegistration const *Next() const noexcept {
    return m_next;
  }

  ReactModuleInfo const &ModuleInfo() const noexcept {
    return m_moduleInfo;
  }

 private:
  ReactModuleInfo const m_moduleInfo;
  ReactModuleRegistration const *m_next{nullptr};

  static const ReactModuleRegistration *s_head;
};

// Adds all registered modules to the package builder.
void AddAttributedModules(ReactPackageBuilder const &packageBuilder) noexcept;
void AddAttributedModules(IReactPackageBuilder const &packageBuilder) noexcept;

// Tries to add a registered module with the moduleName. It returns true if succeeded, or false otherwise.
bool TryAddAttributedModule(ReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept;
bool TryAddAttributedModule(IReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept;

} // namespace winrt::Microsoft::ReactNative
