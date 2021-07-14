// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include <JSI/JsiApiContext.h>
#include <NativeModules.h>
#include <winrt/Windows.System.h>
#include "TestEventService.h"
#include "TestReactNativeHostHolder.h"

using namespace facebook::jsi;
using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::System;

namespace ReactNativeIntegrationTests {

// Use anonymous namespace to avoid any linking conflicts
namespace {

// The default dispatcher is the JSDispatcher.
REACT_MODULE(TestDispatchedModule1)
struct TestDispatchedModule1 {
  REACT_INIT(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    // Check that the method is run in the JS dispatcher thread.
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule1::Initialize", nullptr);
  }

  REACT_METHOD(TestCheckDispatcher, L"testCheckDispatcher")
  void TestCheckDispatcher() noexcept {
    // Check that the method is run in the JS dispatcher thread.
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheck(!m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule1::testCheckDispatcher completed", nullptr);
  }

 private:
  ReactContext m_reactContext;
};

// The UIDispatcher is the STA thread dispatcher. The UIDispatcher is a special predefined
// alias for ReactDispatcherHelper::UIDispatcherProperty() that only can be used in the REACT_MODULE macro.
REACT_MODULE(TestDispatchedModule2, dispatcherName = UIDispatcher)
struct TestDispatchedModule2 {
  REACT_INIT(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    // Check that the method is run in the JS dispatcher thread.
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule2::Initialize", nullptr);
  }

  REACT_METHOD(TestCheckDispatcher, L"testCheckDispatcher")
  void TestCheckDispatcher() noexcept {
    // Check that the method is run in the UI dispatcher thread.
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestCheck(!m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule2::testCheckDispatcher completed", nullptr);
  }

 private:
  ReactContext m_reactContext;
};

// The JSDispatcher is the default dispatcher. The JSDispatcher is a special predefined
// alias for ReactDispatcherHelper::JSDispatcherProperty() that only can be used in the REACT_MODULE macro.
REACT_MODULE(TestDispatchedModule3, dispatcherName = JSDispatcher)
struct TestDispatchedModule3 {
  REACT_INIT(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    // Check that the method is run in the JS dispatcher thread.
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule3::Initialize", nullptr);
  }

  REACT_METHOD(TestCheckDispatcher, L"testCheckDispatcher")
  void TestCheckDispatcher() noexcept {
    // Check that the method is run in the JS dispatcher thread.
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheck(!m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule3::testCheckDispatcher completed", nullptr);
  }

 private:
  ReactContext m_reactContext;
};

// Define the custom dispatcher property Id: it encapsulates the property name and type.
ReactPropertyId<IReactDispatcher> CustomDispatcherId() noexcept {
  static auto dispatcherId = ReactPropertyId<IReactDispatcher>(L"ReactNativeIntegrationTests", L"CustomDispatcher");
  return dispatcherId;
}

// Use custom dispatcher to run tasks sequentially in background threads.
// We must provide the IReactPropertyName as a value to dispatcherName argument.
REACT_MODULE(TestDispatchedModule4, dispatcherName = CustomDispatcherId().Handle())
struct TestDispatchedModule4 {
  REACT_INIT(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    // Check that the method is run in the JS dispatcher thread.
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule4::Initialize", nullptr);
  }

  REACT_METHOD(TestCheckDispatcher, L"testCheckDispatcher")
  void TestCheckDispatcher() noexcept {
    // Check that the method is run in the custom dispatcher thread, and not in JS or UI dispatcher threads.
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestCheck(!m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheck(!m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("TestDispatchedModule4::testCheckDispatcher completed", nullptr);
  }

 private:
  ReactContext m_reactContext;
};

struct TestPackageProvider : ReactPackageProvider<TestPackageProvider> {
  void CreatePackage(IReactPackageBuilder const &packageBuilder) noexcept {
    AddModule<TestDispatchedModule1>(packageBuilder);
    AddModule<TestDispatchedModule2>(packageBuilder);
    AddModule<TestDispatchedModule3>(packageBuilder);
    AddModule<TestDispatchedModule4>(packageBuilder);
  }
};

} // namespace

TEST_CLASS (DispatchedNativeModuleTests) {
  TEST_METHOD(Run_JSDrivenTests) {
    TestEventService::Initialize();

    // Simulate the UI thread using DispatcherQueueController
    auto queueController = DispatcherQueueController::CreateOnDedicatedThread();
    auto uiDispatcher = queueController.DispatcherQueue();
    std::unique_ptr<TestReactNativeHostHolder> reactNativeHost;
    uiDispatcher.TryEnqueue([&]() noexcept {
      reactNativeHost = std::make_unique<TestReactNativeHostHolder>(
          L"DispatchedNativeModuleTests", [](ReactNativeHost const &host) noexcept {
            host.PackageProviders().Append(winrt::make<TestPackageProvider>());
            // Create and store custom dispatcher in the instance property bag. 
            host.InstanceSettings().Properties().Set(
                CustomDispatcherId().Handle(), ReactDispatcher::CreateSerialDispatcher().Handle());
          });
    });

    TestEventService::ObserveEvents({
        TestEvent{"TestDispatchedModule1::Initialize", nullptr},
        TestEvent{"TestDispatchedModule2::Initialize", nullptr},
        TestEvent{"TestDispatchedModule3::Initialize", nullptr},
        TestEvent{"TestDispatchedModule4::Initialize", nullptr},
        TestEvent{"TestDispatchedModule2::testCheckDispatcher completed", nullptr},
        TestEvent{"TestDispatchedModule4::testCheckDispatcher completed", nullptr},
        TestEvent{"TestDispatchedModule1::testCheckDispatcher completed", nullptr},
        TestEvent{"TestDispatchedModule3::testCheckDispatcher completed", nullptr},
    });

    queueController.ShutdownQueueAsync().get();
  }
};

} // namespace ReactNativeIntegrationTests
