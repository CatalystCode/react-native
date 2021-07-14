// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReactInstanceWin.h"
#include "MoveOnCopy.h"
#include "MsoUtils.h"

#include <Base/CoreNativeModules.h>
#include <Threading/MessageDispatchQueue.h>
#include <Threading/MessageQueueThreadFactory.h>
#include <comUtil/qiCast.h>

#ifndef CORE_ABI
#include <XamlUIService.h>
#endif
#include "ReactErrorProvider.h"

#include "Microsoft.ReactNative/IReactNotificationService.h"

#include "../../codegen/NativeAccessibilityInfoSpec.g.h"
#include "../../codegen/NativeAppStateSpec.g.h"
#include "../../codegen/NativeClipboardSpec.g.h"
#include "../../codegen/NativeDevSettingsSpec.g.h"
#include "../../codegen/NativeDeviceInfoSpec.g.h"
#include "../../codegen/NativeI18nManagerSpec.g.h"
#include "../../codegen/NativeLogBoxSpec.g.h"
#include "../../codegen/NativeUIManagerSpec.g.h"
#include "NativeModules.h"
#include "NativeModulesProvider.h"
#include "Unicode.h"

#ifdef USE_FABRIC
#include <Fabric/FabricUIManagerModule.h>
#include <SchedulerSettings.h>
#endif
#include <JSCallInvokerScheduler.h>
#include <QuirkSettings.h>
#include <Shared/DevServerHelper.h>
#include <Views/ViewManager.h>
#include <base/CoreNativeModules.h>
#include <dispatchQueue/dispatchQueue.h>
#ifndef CORE_ABI
#include "ConfigureBundlerDlg.h"
#include "DevMenu.h"
#endif
#include "IReactContext.h"
#include "IReactDispatcher.h"
#ifndef CORE_ABI
#include "Modules/AccessibilityInfoModule.h"
#include "Modules/AlertModule.h"
#include "Modules/AppStateModule.h"
#include "Modules/ClipboardModule.h"
#endif
#include "Modules/DevSettingsModule.h"
#ifndef CORE_ABI
#include "Modules/DeviceInfoModule.h"
#include "Modules/I18nManagerModule.h"
#include "Modules/LogBoxModule.h"
#include "Modules/NativeUIManager.h"
#include "Modules/PaperUIManagerModule.h"
#endif
#include "Modules/ReactRootViewTagGenerator.h"

#ifndef CORE_ABI
#include <Utils/UwpPreparedScriptStore.h>
#include <Utils/UwpScriptStore.h>
#endif

#if defined(INCLUDE_HERMES)
#include "HermesRuntimeHolder.h"
#endif // INCLUDE_HERMES

#if defined(USE_V8)
#include <winrt/Windows.Storage.h>
#include "BaseScriptStoreImpl.h"
#include "V8JSIRuntimeHolder.h"
#endif // USE_V8

#include "RedBox.h"

#include <tuple>
#include "ChakraRuntimeHolder.h"

#include "JsiApi.h"
#include "ReactCoreInjection.h"

namespace Microsoft::ReactNative {

void AddStandardViewManagers(
    std::vector<std::unique_ptr<Microsoft::ReactNative::IViewManager>> &viewManagers,
    const Mso::React::IReactContext &context) noexcept;

std::shared_ptr<facebook::react::IUIManager> CreateUIManager2(
    Mso::React::IReactContext *context,
    std::vector<Microsoft::ReactNative::IViewManager> &&viewManagers) noexcept;

} // namespace Microsoft::ReactNative

using namespace winrt::Microsoft::ReactNative;

namespace Mso::React {

//=============================================================================================
// LoadedCallbackGuard ensures that the OnReactInstanceLoaded is always called.
// It calls OnReactInstanceLoaded in destructor with a cancellation error.
// If loading was previously succeeded this call with an error code is ignored.
//=============================================================================================

struct LoadedCallbackGuard {
  LoadedCallbackGuard(ReactInstanceWin &reactInstance) noexcept : m_reactInstance{&reactInstance} {}

  LoadedCallbackGuard(const LoadedCallbackGuard &other) = delete;
  LoadedCallbackGuard &operator=(const LoadedCallbackGuard &other) = delete;

  LoadedCallbackGuard(LoadedCallbackGuard &&other) = default;
  LoadedCallbackGuard &operator=(LoadedCallbackGuard &&other) = default;

  ~LoadedCallbackGuard() noexcept {
    if (m_reactInstance) {
      m_reactInstance->OnReactInstanceLoaded(Mso::CancellationErrorProvider().MakeErrorCode(true));
    }
  }

 private:
  Mso::CntPtr<ReactInstanceWin> m_reactInstance;
};

struct BridgeUIBatchInstanceCallback final : public facebook::react::InstanceCallback {
  BridgeUIBatchInstanceCallback(Mso::WeakPtr<ReactInstanceWin> wkInstance) : m_wkInstance(wkInstance) {}
  virtual ~BridgeUIBatchInstanceCallback() = default;
  void onBatchComplete() override {
    if (auto instance = m_wkInstance.GetStrongPtr()) {
      if (instance->IsLoaded()) {
        if (instance->UseWebDebugger()) {
          // While using a CxxModule for UIManager (which we do when running under webdebugger)
          // We need to post the batch complete to the NativeQueue to ensure that the UIManager
          // has posted everything from this batch into its queue before we complete the batch.
          instance->m_jsDispatchQueue.Load().Post([wkInstance = m_wkInstance]() {
            if (auto instance = wkInstance.GetStrongPtr()) {
              instance->m_batchingUIThread->runOnQueue([wkInstance]() {
                if (auto instance = wkInstance.GetStrongPtr()) {
                  auto propBag = ReactPropertyBag(instance->m_reactContext->Properties());
                  if (auto callback = propBag.Get(winrt::Microsoft::ReactNative::implementation::ReactCoreInjection::
                                                      UIBatchCompleteCallbackProperty())) {
                    (*callback)(instance->m_reactContext->Properties());
                  }
#ifndef CORE_ABI
                  if (auto uiManager = Microsoft::ReactNative::GetNativeUIManager(*instance->m_reactContext).lock()) {
                    uiManager->onBatchComplete();
                  }
#endif
                }
              });

              // For UWP we use a batching message queue to optimize the usage
              // of the CoreDispatcher.  Win32 already has an optimized queue.
              facebook::react::BatchingMessageQueueThread *batchingUIThread =
                  static_cast<facebook::react::BatchingMessageQueueThread *>(instance->m_batchingUIThread.get());
              if (batchingUIThread != nullptr) {
                batchingUIThread->onBatchComplete();
              }
            }
          });
        } else {
          instance->m_batchingUIThread->runOnQueue([wkInstance = m_wkInstance]() {
            if (auto instance = wkInstance.GetStrongPtr()) {
              auto propBag = ReactPropertyBag(instance->m_reactContext->Properties());
              if (auto callback = propBag.Get(winrt::Microsoft::ReactNative::implementation::ReactCoreInjection::
                                                  UIBatchCompleteCallbackProperty())) {
                (*callback)(instance->m_reactContext->Properties());
              }
#ifndef CORE_ABI
              if (auto uiManager = Microsoft::ReactNative::GetNativeUIManager(*instance->m_reactContext).lock()) {
                uiManager->onBatchComplete();
              }
#endif
            }
          });
          // For UWP we use a batching message queue to optimize the usage
          // of the CoreDispatcher.  Win32 already has an optimized queue.
          facebook::react::BatchingMessageQueueThread *batchingUIThread =
              static_cast<facebook::react::BatchingMessageQueueThread *>(instance->m_batchingUIThread.get());
          if (batchingUIThread != nullptr) {
            batchingUIThread->onBatchComplete();
          }
        }
      }
    }
  }
  void incrementPendingJSCalls() override {}
  void decrementPendingJSCalls() override {}

  Mso::WeakPtr<ReactInstanceWin> m_wkInstance;
  Mso::CntPtr<Mso::React::ReactContext> m_context;
  std::weak_ptr<facebook::react::MessageQueueThread> m_uiThread;
};

//=============================================================================================
// ReactInstanceWin implementation
//=============================================================================================

ReactInstanceWin::ReactInstanceWin(
    IReactHost &reactHost,
    ReactOptions const &options,
    Mso::Promise<void> &&whenCreated,
    Mso::Promise<void> &&whenLoaded,
    Mso::VoidFunctor &&updateUI) noexcept
    : Super{reactHost.NativeQueue()},
      m_weakReactHost{&reactHost},
      m_options{options},
      m_whenCreated{std::move(whenCreated)},
      m_isFastReloadEnabled(options.UseFastRefresh()),
      m_isLiveReloadEnabled(options.UseLiveReload()),
      m_updateUI{std::move(updateUI)},
      m_useWebDebugger(options.UseWebDebugger()),
      m_useDirectDebugger(options.UseDirectDebugger()),
      m_debuggerBreakOnNextLine(options.DebuggerBreakOnNextLine()),
      m_reactContext{Mso::Make<ReactContext>(
          this,
          options.Properties,
          winrt::make<implementation::ReactNotificationService>(options.Notifications))} {
  // As soon as the bundle is loaded or failed to load, we set the m_whenLoaded promise value in JS queue.
  // It then synchronously raises the OnInstanceLoaded event in the JS queue.
  // Then, we notify the ReactHost about the load event in the internal queue.
  m_whenLoaded.AsFuture()
      .Then<Mso::Executors::Inline>(
          [onLoaded = m_options.OnInstanceLoaded, reactContext = m_reactContext](Mso::Maybe<void> &&value) noexcept {
            if (onLoaded) {
              onLoaded.Get()->Invoke(reactContext, value.IsError() ? value.TakeError() : Mso::ErrorCode());
            }
          })
      .Then(Queue(), [whenLoaded = std::move(whenLoaded)](Mso::Maybe<void> &&value) noexcept {
        whenLoaded.SetValue(std::move(value));
      });

  // When the JS queue is shutdown, we set the m_whenDestroyed promise value as the last work item in the JS queue.
  // No JS queue work can be done after that for the instance.
  // The promise continuation synchronously calls the OnInstanceDestroyed event.
  // Then, the Destroy() method returns the m_whenDestroyedResult future to ReactHost to handle instance destruction.
  m_whenDestroyedResult =
      m_whenDestroyed.AsFuture().Then<Mso::Executors::Inline>([whenLoaded = m_whenLoaded,
                                                               onDestroyed = m_options.OnInstanceDestroyed,
                                                               reactContext = m_reactContext]() noexcept {
        whenLoaded.TryCancel(); // It only has an effect if whenLoaded was not set before
        if (onDestroyed) {
          onDestroyed.Get()->Invoke(reactContext);
        }
      });

  // We notify the ReactHost immediately that the instance is created, but the
  // OnInstanceCreated event is raised only after the internal react-native instance is ready and
  // it starts handling JS queue work items.
  m_whenCreated.SetValue();
}

ReactInstanceWin::~ReactInstanceWin() noexcept {}

void ReactInstanceWin::LoadModules(
    const std::shared_ptr<winrt::Microsoft::ReactNative::NativeModulesProvider> &nativeModulesProvider,
    const std::shared_ptr<winrt::Microsoft::ReactNative::TurboModulesProvider> &turboModulesProvider) noexcept {
  auto registerNativeModule = [&nativeModulesProvider](
                                  const wchar_t *name, const ReactModuleProvider &provider) noexcept {
    nativeModulesProvider->AddModuleProvider(name, provider);
  };

  auto registerTurboModule = [this, &nativeModulesProvider, &turboModulesProvider](
                                 const wchar_t *name, const ReactModuleProvider &provider) noexcept {
    if (m_options.UseWebDebugger()) {
      nativeModulesProvider->AddModuleProvider(name, provider);
    } else {
      turboModulesProvider->AddModuleProvider(name, provider);
    }
  };

#ifdef USE_FABRIC
  if (!m_options.UseWebDebugger()) {
    registerTurboModule(
        L"FabricUIManagerBinding",
        winrt::Microsoft::ReactNative::MakeModuleProvider<::Microsoft::ReactNative::FabricUIManager>());
  }
#endif

#ifndef CORE_ABI
  registerTurboModule(
      L"UIManager",
      // Spec incorrectly reports commandID as a number, but its actually a number | string.. so dont use the spec for
      // now
      // winrt::Microsoft::ReactNative::MakeTurboModuleProvider < ::Microsoft::ReactNative::UIManager,
      //::Microsoft::ReactNativeSpecs::UIManagerSpec>());
      winrt::Microsoft::ReactNative::MakeModuleProvider<::Microsoft::ReactNative::UIManager>());

  registerTurboModule(
      L"AccessibilityInfo",
      winrt::Microsoft::ReactNative::MakeTurboModuleProvider<
          ::Microsoft::ReactNative::AccessibilityInfo,
          ::Microsoft::ReactNativeSpecs::AccessibilityInfoSpec>());

  registerTurboModule(L"Alert", winrt::Microsoft::ReactNative::MakeModuleProvider<::Microsoft::ReactNative::Alert>());

  registerTurboModule(
      L"AppState",
      winrt::Microsoft::ReactNative::
          MakeTurboModuleProvider<::Microsoft::ReactNative::AppState, ::Microsoft::ReactNativeSpecs::AppStateSpec>());

  registerTurboModule(
      L"LogBox",
      winrt::Microsoft::ReactNative::
          MakeTurboModuleProvider<::Microsoft::ReactNative::LogBox, ::Microsoft::ReactNativeSpecs::LogBoxSpec>());

  registerTurboModule(
      L"Clipboard",
      winrt::Microsoft::ReactNative::
          MakeTurboModuleProvider<::Microsoft::ReactNative::Clipboard, ::Microsoft::ReactNativeSpecs::ClipboardSpec>());

  registerTurboModule(
      L"DeviceInfo",
      winrt::Microsoft::ReactNative::MakeTurboModuleProvider<
          ::Microsoft::ReactNative::DeviceInfo,
          ::Microsoft::ReactNativeSpecs::DeviceInfoSpec>());
#endif

  registerTurboModule(
      L"DevSettings",
      winrt::Microsoft::ReactNative::MakeTurboModuleProvider<
          ::Microsoft::ReactNative::DevSettings,
          ::Microsoft::ReactNativeSpecs::DevSettingsSpec>());

#ifndef CORE_ABI
  registerTurboModule(
      L"I18nManager",
      winrt::Microsoft::ReactNative::MakeTurboModuleProvider<
          ::Microsoft::ReactNative::I18nManager,
          ::Microsoft::ReactNativeSpecs::I18nManagerSpec>());
#endif
}

//! Initialize() is called from the native queue.
void ReactInstanceWin::Initialize() noexcept {
  InitJSMessageThread();
  InitNativeMessageThread();
  InitUIMessageThread();

#ifndef CORE_ABI
  // InitUIManager uses m_legacyReactInstance
  InitUIManager();

  Microsoft::ReactNative::DevMenuManager::InitDevMenu(m_reactContext, [weakReactHost = m_weakReactHost]() noexcept {
    Microsoft::ReactNative::ShowConfigureBundlerDialog(weakReactHost);
  });
#endif

  Mso::PostFuture(m_uiQueue, [weakThis = Mso::WeakPtr{this}]() noexcept {
#ifndef CORE_ABI
    // Objects that must be created on the UI thread
    if (auto strongThis = weakThis.GetStrongPtr()) {
      strongThis->m_appTheme = std::make_shared<Microsoft::ReactNative::AppTheme>(
          strongThis->GetReactContext(), strongThis->m_uiMessageThread.LoadWithLock());
      Microsoft::ReactNative::I18nManager::InitI18nInfo(
          winrt::Microsoft::ReactNative::ReactPropertyBag(strongThis->Options().Properties));
      strongThis->m_appearanceListener = Mso::Make<Microsoft::ReactNative::AppearanceChangeListener>(
          strongThis->GetReactContext(), strongThis->m_uiQueue);

      Microsoft::ReactNative::DeviceInfoHolder::InitDeviceInfoHolder(strongThis->GetReactContext());
    }
#endif
  }).Then(Queue(), [this, weakThis = Mso::WeakPtr{this}]() noexcept {
    if (auto strongThis = weakThis.GetStrongPtr()) {
      // auto cxxModulesProviders = GetCxxModuleProviders();

      auto devSettings = std::make_shared<facebook::react::DevSettings>();
      devSettings->useJITCompilation = m_options.EnableJITCompilation;
      devSettings->sourceBundleHost = SourceBundleHost();
      devSettings->sourceBundlePort = SourceBundlePort();
      devSettings->debugBundlePath = DebugBundlePath();
      devSettings->liveReloadCallback = GetLiveReloadCallback();
      devSettings->errorCallback = GetErrorCallback();
      devSettings->loggingCallback = GetLoggingCallback();
      m_redboxHandler = devSettings->redboxHandler = std::move(GetRedBoxHandler());
      devSettings->useDirectDebugger = m_useDirectDebugger;
      devSettings->debuggerBreakOnNextLine = m_debuggerBreakOnNextLine;
      devSettings->debuggerPort = m_options.DeveloperSettings.DebuggerPort;
      devSettings->debuggerRuntimeName = m_options.DeveloperSettings.DebuggerRuntimeName;
      devSettings->useWebDebugger = m_useWebDebugger;
      devSettings->useFastRefresh = m_isFastReloadEnabled;
      // devSettings->memoryTracker = GetMemoryTracker();
      devSettings->bundleRootPath = BundleRootPath();

      devSettings->waitingForDebuggerCallback = GetWaitingForDebuggerCallback();
      devSettings->debuggerAttachCallback = GetDebuggerAttachCallback();
#ifndef CORE_ABI
      devSettings->showDevMenuCallback = [weakThis]() noexcept {
        if (auto strongThis = weakThis.GetStrongPtr()) {
          strongThis->m_uiQueue.Post([context = strongThis->m_reactContext]() {
            Microsoft::ReactNative::DevMenuManager::Show(context->Properties());
          });
        }
      };
#endif

      // Now that ReactNativeWindows is building outside devmain, it is missing
      // fix given by PR https://github.com/microsoft/react-native-windows/pull/2624 causing
      // regression. We're turning off console redirection till the fix is available in devmain.
      // Bug https://office.visualstudio.com/DefaultCollection/OC/_workitems/edit/3441551 is tracking this
      devSettings->debuggerConsoleRedirection = false; // JSHost::ChangeGate::ChakraCoreDebuggerConsoleRedirection();

#ifdef CORE_ABI
      std::vector<facebook::react::NativeModuleDescription> cxxModules;
#else
      // Acquire default modules and then populate with custom modules.
      // Note that some of these have custom thread affinity.
      std::vector<facebook::react::NativeModuleDescription> cxxModules = Microsoft::ReactNative::GetCoreModules(
          m_batchingUIThread,
          m_jsMessageThread.Load(),
          std::move(m_appTheme),
          std::move(m_appearanceListener),
          m_reactContext);
#endif

      auto nmp = std::make_shared<winrt::Microsoft::ReactNative::NativeModulesProvider>();

      LoadModules(nmp, m_options.TurboModuleProvider);

      auto modules = nmp->GetModules(m_reactContext, m_jsMessageThread.Load());
      cxxModules.insert(
          cxxModules.end(), std::make_move_iterator(modules.begin()), std::make_move_iterator(modules.end()));

      if (m_options.ModuleProvider != nullptr) {
        std::vector<facebook::react::NativeModuleDescription> customCxxModules =
            m_options.ModuleProvider->GetModules(m_reactContext, m_jsMessageThread.Load());
        cxxModules.insert(std::end(cxxModules), std::begin(customCxxModules), std::end(customCxxModules));
      }

      std::unique_ptr<facebook::jsi::ScriptStore> scriptStore = nullptr;
      std::unique_ptr<facebook::jsi::PreparedScriptStore> preparedScriptStore = nullptr;

      switch (m_options.JsiEngine) {
        case JSIEngine::Hermes:
#if defined(INCLUDE_HERMES)
          devSettings->jsiRuntimeHolder =
              std::make_shared<facebook::react::HermesRuntimeHolder>(devSettings, m_jsMessageThread.Load());
          devSettings->inlineSourceMap = false;
          break;
#endif
        case JSIEngine::V8:
#if defined(USE_V8)
#ifndef CORE_ABI
          preparedScriptStore =
              std::make_unique<facebook::react::BasePreparedScriptStoreImpl>(getApplicationLocalFolder());
#endif // CORE_ABI
          devSettings->jsiRuntimeHolder = std::make_shared<facebook::react::V8JSIRuntimeHolder>(
              devSettings, m_jsMessageThread.Load(), std::move(scriptStore), std::move(preparedScriptStore));
          break;
#endif // USE_V8
        case JSIEngine::Chakra:
#ifndef CORE_ABI
          if (m_options.EnableByteCodeCaching || !m_options.ByteCodeFileUri.empty()) {
            scriptStore = std::make_unique<Microsoft::ReactNative::UwpScriptStore>();
            preparedScriptStore = std::make_unique<Microsoft::ReactNative::UwpPreparedScriptStore>(
                winrt::to_hstring(m_options.ByteCodeFileUri));
          }
#endif
          devSettings->jsiRuntimeHolder = std::make_shared<Microsoft::JSI::ChakraRuntimeHolder>(
              devSettings, m_jsMessageThread.Load(), std::move(scriptStore), std::move(preparedScriptStore));
          break;
      }

      m_jsiRuntimeHolder = devSettings->jsiRuntimeHolder;

      try {
        // We need to keep the instance wrapper alive as its destruction shuts down the native queue.
        m_options.TurboModuleProvider->SetReactContext(
            winrt::make<implementation::ReactContext>(Mso::Copy(m_reactContext)));
        auto bundleRootPath = devSettings->bundleRootPath;
        auto instanceWrapper = facebook::react::CreateReactInstance(
            std::shared_ptr<facebook::react::Instance>(strongThis->m_instance.Load()),
            std::move(bundleRootPath), // bundleRootPath
            std::move(cxxModules),
            m_options.TurboModuleProvider,
            std::make_unique<BridgeUIBatchInstanceCallback>(weakThis),
            m_jsMessageThread.Load(),
            Mso::Copy(m_batchingUIThread),
            std::move(devSettings));

        m_instanceWrapper.Exchange(std::move(instanceWrapper));

#ifdef USE_FABRIC
        // Eagerly init the FabricUI binding
        if (!m_options.UseWebDebugger()) {
          Microsoft::ReactNative::SchedulerSettings::SetRuntimeExecutor(
              winrt::Microsoft::ReactNative::ReactPropertyBag(m_reactContext->Properties()),
              m_instanceWrapper.Load()->GetInstance()->getRuntimeExecutor());
          m_options.TurboModuleProvider->getModule("FabricUIManagerBinding", m_instance.Load()->getJSCallInvoker());
        }
#endif

        LoadJSBundles();

        if (UseDeveloperSupport() && State() != ReactInstanceState::HasError) {
          folly::dynamic params = folly::dynamic::array(
              STRING(RN_PLATFORM), DebugBundlePath(), SourceBundleHost(), SourceBundlePort(), m_isFastReloadEnabled);
          m_instance.Load()->callJSFunction("HMRClient", "setup", std::move(params));
        }

      } catch (std::exception &e) {
        OnErrorWithMessage(e.what());
        OnErrorWithMessage("UwpReactInstance: Failed to create React Instance.");
      } catch (winrt::hresult_error const &e) {
        OnErrorWithMessage(Microsoft::Common::Unicode::Utf16ToUtf8(e.message().c_str(), e.message().size()));
        OnErrorWithMessage("UwpReactInstance: Failed to create React Instance.");
      } catch (...) {
        OnErrorWithMessage("UwpReactInstance: Failed to create React Instance.");
      }
    }
  });
}

void ReactInstanceWin::LoadJSBundles() noexcept {
  //
  // We use m_jsMessageThread to load JS bundles synchronously. In that case we only load
  // them if the m_jsMessageThread is not shut down (quitSynchronous() is not called).
  // After the load we call OnReactInstanceLoaded callback on native queue.
  //
  // Note that the instance could be destroyed while we are loading JS Bundles.
  // Though, the JS engine is not destroyed until this work item is not finished.
  // Thus, we check the m_isDestroyed flag to see if we should do an early exit.
  // Also, since we have to guarantee that the OnReactInstanceLoaded callback is called before
  // the OnReactInstanceDestroyed callback, the OnReactInstanceLoaded is called right before the
  // OnReactInstanceDestroyed callback in the Destroy() method. In that case any OnReactInstanceLoaded
  // calls after we finish this JS message queue work item is ignored.
  //
  // The LoadedCallbackGuard is used for the case when runOnQueue does not execute the lambda
  // before destroying it. It may happen if the m_jsMessageThread is already shutdown.
  // In that case, the LoadedCallbackGuard notifies about cancellation by calling OnReactInstanceLoaded.
  // The OnReactInstanceLoaded internally only accepts the first call and ignores others.
  //

  if (m_useWebDebugger || m_isFastReloadEnabled) {
    // Getting bundle from the packager, so do everything async.
    auto instanceWrapper = m_instanceWrapper.LoadWithLock();
    instanceWrapper->loadBundle(Mso::Copy(JavaScriptBundleFile()));

    m_jsMessageThread.Load()->runOnQueue(
        [weakThis = Mso::WeakPtr{this},
         loadCallbackGuard = Mso::MakeMoveOnCopyWrapper(LoadedCallbackGuard{*this})]() noexcept {
          if (auto strongThis = weakThis.GetStrongPtr()) {
            if (strongThis->State() != ReactInstanceState::HasError) {
              strongThis->OnReactInstanceLoaded(Mso::ErrorCode{});
            }
          }
        });
  } else {
    m_jsMessageThread.Load()->runOnQueue(
        [weakThis = Mso::WeakPtr{this},
         loadCallbackGuard = Mso::MakeMoveOnCopyWrapper(LoadedCallbackGuard{*this})]() noexcept {
          if (auto strongThis = weakThis.GetStrongPtr()) {
            auto instance = strongThis->m_instance.LoadWithLock();
            auto instanceWrapper = strongThis->m_instanceWrapper.LoadWithLock();
            if (!instance || !instanceWrapper) {
              return;
            }

            try {
              instanceWrapper->loadBundleSync(Mso::Copy(strongThis->JavaScriptBundleFile()));
              strongThis->OnReactInstanceLoaded(Mso::ErrorCode{});
            } catch (...) {
              strongThis->OnReactInstanceLoaded(Mso::ExceptionErrorProvider().MakeErrorCode(std::current_exception()));
            }
          }
        });
  }
}

void ReactInstanceWin::OnReactInstanceLoaded(const Mso::ErrorCode &errorCode) noexcept {
  bool isLoadedExpected = false;
  if (m_isLoaded.compare_exchange_strong(isLoadedExpected, true)) {
    if (!errorCode) {
      m_state = ReactInstanceState::Loaded;
      m_whenLoaded.SetValue();
      DrainJSCallQueue();
    } else {
      m_state = ReactInstanceState::HasError;
      m_whenLoaded.SetError(errorCode);
      OnError(errorCode);
    }
  }
}

Mso::Future<void> ReactInstanceWin::Destroy() noexcept {
  // This method must be called from the native queue.
  VerifyIsInQueueElseCrash();

  if (m_isDestroyed) {
    return m_whenDestroyedResult;
  }

  m_isDestroyed = true;
  m_state = ReactInstanceState::Unloaded;
  AbandonJSCallQueue();

  // Make sure that the instance is not destroyed yet
  if (auto instance = m_instance.Exchange(nullptr)) {
    {
      // Release the JSI runtime
      std::scoped_lock lock{m_mutex};
      m_jsiRuntimeHolder = nullptr;
      m_jsiRuntime = nullptr;
    }
    // Release the message queues before the ui manager and instance.
    m_nativeMessageThread.Exchange(nullptr);
    m_jsMessageThread.Exchange(nullptr);
    m_instanceWrapper.Exchange(nullptr);
    m_jsDispatchQueue.Exchange(nullptr);
  }

  return m_whenDestroyedResult;
}

const ReactOptions &ReactInstanceWin::Options() const noexcept {
  return m_options;
}

ReactInstanceState ReactInstanceWin::State() const noexcept {
  return m_state;
}

void ReactInstanceWin::InitJSMessageThread() noexcept {
  m_instance.Exchange(std::make_shared<facebook::react::Instance>());

  auto scheduler = Mso::MakeJSCallInvokerScheduler(
      m_instance.Load()->getJSCallInvoker(),
      Mso::MakeWeakMemberFunctor(this, &ReactInstanceWin::OnError),
      Mso::Copy(m_whenDestroyed));
  auto jsDispatchQueue = Mso::DispatchQueue::MakeCustomQueue(Mso::CntPtr(scheduler));

  // This work item will be processed as a first item in JS queue when the react instance is created.
  jsDispatchQueue.Post([onCreated = m_options.OnInstanceCreated, reactContext = m_reactContext]() noexcept {
    if (onCreated) {
      onCreated.Get()->Invoke(reactContext);
    }
  });

  auto jsDispatcher =
      winrt::make<winrt::Microsoft::ReactNative::implementation::ReactDispatcher>(Mso::Copy(jsDispatchQueue));
  m_options.Properties.Set(ReactDispatcherHelper::JSDispatcherProperty(), jsDispatcher);

  m_jsMessageThread.Exchange(qi_cast<Mso::IJSCallInvokerQueueScheduler>(scheduler.Get())->GetMessageQueue());
  m_jsDispatchQueue.Exchange(std::move(jsDispatchQueue));
}

void ReactInstanceWin::InitNativeMessageThread() noexcept {
  // Native queue was already given us in constructor.
  m_nativeMessageThread.Exchange(
      std::make_shared<MessageDispatchQueue>(Queue(), Mso::MakeWeakMemberFunctor(this, &ReactInstanceWin::OnError)));
}

void ReactInstanceWin::InitUIMessageThread() noexcept {
  // Native queue was already given us in constructor.
  m_uiQueue = winrt::Microsoft::ReactNative::implementation::ReactDispatcher::GetUIDispatchQueue(m_options.Properties);
  VerifyElseCrashSz(m_uiQueue, "No UI Dispatcher provided");
  m_uiMessageThread.Exchange(
      std::make_shared<MessageDispatchQueue>(m_uiQueue, Mso::MakeWeakMemberFunctor(this, &ReactInstanceWin::OnError)));

  auto batchingUIThread = Microsoft::ReactNative::MakeBatchingQueueThread(m_uiMessageThread.Load());
  m_batchingUIThread = batchingUIThread;

  ReactPropertyBag(m_reactContext->Properties())
      .Set(
          winrt::Microsoft::ReactNative::implementation::ReactCoreInjection::PostToUIBatchingQueueProperty(),
          [wkBatchingUIThread = std::weak_ptr<facebook::react::BatchingMessageQueueThread>(batchingUIThread)](
              winrt::Microsoft::ReactNative::ReactDispatcherCallback const &callback) {
            if (auto batchingUIThread = wkBatchingUIThread.lock()) {
              batchingUIThread->runOnQueue(callback);
            }
          });

  m_jsDispatchQueue.Load().Post(
      [batchingUIThread, instance = std::weak_ptr<facebook::react::Instance>(m_instance.Load())]() noexcept {
        batchingUIThread->decoratedNativeCallInvokerReady(instance);
      });
}

#ifndef CORE_ABI
void ReactInstanceWin::InitUIManager() noexcept {
  std::vector<std::unique_ptr<Microsoft::ReactNative::IViewManager>> viewManagers;

  // Custom view managers
  if (m_options.ViewManagerProvider) {
    viewManagers = m_options.ViewManagerProvider->GetViewManagers(m_reactContext);
  }

  Microsoft::ReactNative::AddStandardViewManagers(viewManagers, *m_reactContext);

  auto uiManagerSettings =
      std::make_unique<Microsoft::ReactNative::UIManagerSettings>(m_batchingUIThread, std::move(viewManagers));
  Microsoft::ReactNative::UIManager::SetSettings(m_reactContext->Properties(), std::move(uiManagerSettings));

  m_reactContext->Properties().Set(
      implementation::XamlUIService::XamlUIServiceProperty().Handle(),
      winrt::make<implementation::XamlUIService>(m_reactContext));
}
#endif

facebook::react::NativeLoggingHook ReactInstanceWin::GetLoggingCallback() noexcept {
  if (m_options.OnLogging) {
    return [logCallback = m_options.OnLogging](facebook::react::RCTLogLevel logLevel, const char *message) {
      logCallback(static_cast<LogLevel>(logLevel), message);
    };
  } else {
    // When no logging callback was specified, use a default one in DEBUG builds
#if DEBUG
    return [telemetryTag{JavaScriptBundleFile()}](facebook::react::RCTLogLevel logLevel, const char *message) {
      std::ostringstream ss;
      ss << "ReactNative ['" << telemetryTag << "'] (";
      switch (logLevel) {
        case facebook::react::RCTLogLevel::Trace:
          ss << "trace";
          break;
        case facebook::react::RCTLogLevel::Info:
          ss << "info";
          break;
        case facebook::react::RCTLogLevel::Warning:
          ss << "warning";
          break;
        case facebook::react::RCTLogLevel::Error:
          ss << "error";
          break;
        case facebook::react::RCTLogLevel::Fatal:
          ss << "fatal";
          break;
      }
      ss << "): '" << message << "'\n";
      OutputDebugStringA(ss.str().c_str());
    };
#else
    return facebook::react::NativeLoggingHook{};
#endif
  }
}

std::shared_ptr<IRedBoxHandler> ReactInstanceWin::GetRedBoxHandler() noexcept {
  if (m_options.RedBoxHandler) {
    return m_options.RedBoxHandler;
#ifndef CORE_ABI
  } else if (UseDeveloperSupport()) {
    auto localWkReactHost = m_weakReactHost;
    return CreateDefaultRedBoxHandler(std::move(localWkReactHost), Mso::Copy(m_uiQueue));
#endif
  } else {
    return {};
  }
}

std::function<void()> ReactInstanceWin::GetLiveReloadCallback() noexcept {
  // Live reload is enabled if we provide a callback function.
  if (m_isLiveReloadEnabled || m_isFastReloadEnabled) {
    return Mso::MakeWeakMemberStdFunction(this, &ReactInstanceWin::OnLiveReload);
  }
  return std::function<void()>{};
}

std::string ReactInstanceWin::GetBytecodeFileName() noexcept {
  // use bytecode caching if enabled and not debugging
  // (ChakraCore debugging does not work when bytecode caching is enabled)
  // TODO: implement
  // bool useByteCode = Mso::React::BytecodeOptimizationEnabled() && !m_options.DeveloperSettings.UseDirectDebugger;
  // return useByteCode ? Mso::React::GetBytecodeFilePath(m_options.Identity) : "";
  return "";
}

std::function<void(std::string)> ReactInstanceWin::GetErrorCallback() noexcept {
  return Mso::MakeWeakMemberStdFunction(this, &ReactInstanceWin::OnErrorWithMessage);
}

void ReactInstanceWin::OnErrorWithMessage(const std::string &errorMessage) noexcept {
  OnError(Mso::React::ReactErrorProvider().MakeErrorCode(Mso::React::ReactError{errorMessage.c_str()}));
}

void ReactInstanceWin::OnError(const Mso::ErrorCode &errorCode) noexcept {
  m_state = ReactInstanceState::HasError;
  AbandonJSCallQueue();

  if (m_redboxHandler && m_redboxHandler->isDevSupportEnabled()) {
    ErrorInfo errorInfo;
    errorInfo.Message = errorCode.ToString();
    errorInfo.Id = 0;
    m_redboxHandler->showNewError(std::move(errorInfo), ErrorType::Native);
  }

  InvokeInQueue([this, errorCode]() noexcept { m_options.OnError(errorCode); });

  m_updateUI();
}

void ReactInstanceWin::OnLiveReload() noexcept {
  if (auto reactHost = m_weakReactHost.GetStrongPtr()) {
    reactHost->ReloadInstance();
  }
}

std::function<void()> ReactInstanceWin::GetWaitingForDebuggerCallback() noexcept {
  if (m_useWebDebugger) {
    return Mso::MakeWeakMemberStdFunction(this, &ReactInstanceWin::OnWaitingForDebugger);
  }

  return {};
}

void ReactInstanceWin::OnWaitingForDebugger() noexcept {
  auto state = m_state.load();
  while (state == ReactInstanceState::Loading) {
    if (m_state.compare_exchange_weak(state, ReactInstanceState::WaitingForDebugger)) {
      break;
    }
  }

  m_updateUI();
}

std::function<void()> ReactInstanceWin::GetDebuggerAttachCallback() noexcept {
  if (m_useWebDebugger) {
    return Mso::MakeWeakMemberStdFunction(this, &ReactInstanceWin::OnDebuggerAttach);
  }

  return {};
}

void ReactInstanceWin::OnDebuggerAttach() noexcept {
  m_updateUI();
}

void ReactInstanceWin::DrainJSCallQueue() noexcept {
  // Handle all items in the queue one by one.
  for (;;) {
    JSCallEntry entry; // To avoid callJSFunction under the lock
    {
      std::scoped_lock lock{m_mutex};
      if (m_state == ReactInstanceState::Loaded && !m_jsCallQueue.empty()) {
        entry = std::move(m_jsCallQueue.front());
        m_jsCallQueue.pop_front();
      } else {
        break;
      }
    }

    if (auto instance = m_instance.LoadWithLock()) {
      instance->callJSFunction(std::move(entry.ModuleName), std::move(entry.MethodName), std::move(entry.Args));
    }
  }
}

void ReactInstanceWin::AbandonJSCallQueue() noexcept {
  std::deque<JSCallEntry> jsCallQueue; // To avoid destruction under the lock
  {
    std::scoped_lock lock{m_mutex};
    if (m_state == ReactInstanceState::HasError || m_state == ReactInstanceState::Unloaded) {
      jsCallQueue = std::move(m_jsCallQueue);
    }
  }
}

void ReactInstanceWin::CallJsFunction(
    std::string &&moduleName,
    std::string &&method,
    folly::dynamic &&params) noexcept {
  bool shouldCall{false}; // To call callJSFunction outside of lock
  {
    std::scoped_lock lock{m_mutex};
    if (m_state == ReactInstanceState::Loaded && m_jsCallQueue.empty()) {
      shouldCall = true;
    } else if (
        m_state == ReactInstanceState::Loading || m_state == ReactInstanceState::WaitingForDebugger ||
        (m_state == ReactInstanceState::Loaded && !m_jsCallQueue.empty())) {
      m_jsCallQueue.push_back(JSCallEntry{std::move(moduleName), std::move(method), std::move(params)});
    }
    // otherwise ignore the call
  }

  if (shouldCall) {
    if (auto instance = m_instance.LoadWithLock()) {
      instance->callJSFunction(std::move(moduleName), std::move(method), std::move(params));
    }
  }
}

void ReactInstanceWin::DispatchEvent(int64_t viewTag, std::string &&eventName, folly::dynamic &&eventData) noexcept {
  folly::dynamic params = folly::dynamic::array(viewTag, std::move(eventName), std::move(eventData));
  CallJsFunction("RCTEventEmitter", "receiveEvent", std::move(params));
}

winrt::Microsoft::ReactNative::JsiRuntime ReactInstanceWin::JsiRuntime() noexcept {
  std::shared_ptr<facebook::jsi::RuntimeHolderLazyInit> jsiRuntimeHolder;
  {
    std::scoped_lock lock{m_mutex};
    if (m_jsiRuntime) {
      return m_jsiRuntime;
    } else {
      jsiRuntimeHolder = m_jsiRuntimeHolder;
    }
  }

  auto jsiRuntime = jsiRuntimeHolder ? jsiRuntimeHolder->getRuntime() : nullptr;

  {
    std::scoped_lock lock{m_mutex};
    if (!m_jsiRuntime && jsiRuntime) {
      // Set only if other thread did not do it yet.
      m_jsiRuntime =
          winrt::Microsoft::ReactNative::implementation::JsiRuntime::GetOrCreate(jsiRuntimeHolder, jsiRuntime);
    }

    return m_jsiRuntime;
  }
}

std::shared_ptr<facebook::react::Instance> ReactInstanceWin::GetInnerInstance() noexcept {
  return m_instance.LoadWithLock();
}

bool ReactInstanceWin::IsLoaded() const noexcept {
  return m_state == ReactInstanceState::Loaded;
}

void ReactInstanceWin::AttachMeasuredRootView(
    facebook::react::IReactRootView *rootView,
    folly::dynamic &&initialProps,
    bool useFabric) noexcept {
#ifndef CORE_ABI
  if (State() == ReactInstanceState::HasError)
    return;

  int64_t rootTag = -1;

#ifdef USE_FABRIC
  if (useFabric && !m_useWebDebugger) {
    auto uiManager = ::Microsoft::ReactNative::FabricUIManager::FromProperties(
        winrt::Microsoft::ReactNative::ReactPropertyBag(m_reactContext->Properties()));

    auto rootTag = Microsoft::ReactNative::getNextRootViewTag();
    rootView->SetTag(rootTag);
    uiManager->startSurface(rootView, rootTag, rootView->JSComponentName(), std::move(initialProps));

  } else
#endif
  {
    if (auto uiManager = Microsoft::ReactNative::GetNativeUIManager(*m_reactContext).lock()) {
      rootTag = uiManager->AddMeasuredRootView(rootView);
      rootView->SetTag(rootTag);
    } else {
      assert(false);
    }

    std::string jsMainModuleName = rootView->JSComponentName();
    folly::dynamic params = folly::dynamic::array(
        std::move(jsMainModuleName),
        folly::dynamic::object("initialProps", std::move(initialProps))("rootTag", rootTag)("fabric", false));
    CallJsFunction("AppRegistry", "runApplication", std::move(params));
  }
#endif
}

void ReactInstanceWin::DetachRootView(facebook::react::IReactRootView *rootView, bool useFabric) noexcept {
  if (State() == ReactInstanceState::HasError)
    return;

  auto rootTag = rootView->GetTag();
  folly::dynamic params = folly::dynamic::array(rootTag);

#ifdef USE_FABRIC
  if (useFabric && !m_useWebDebugger) {
    auto uiManager = ::Microsoft::ReactNative::FabricUIManager::FromProperties(
        winrt::Microsoft::ReactNative::ReactPropertyBag(m_reactContext->Properties()));
    uiManager->stopSurface(static_cast<facebook::react::SurfaceId>(rootTag));

    CallJsFunction("ReactFabric", "unmountComponentAtNode", std::move(params));
  } else
#endif
  {
    CallJsFunction("AppRegistry", "unmountApplicationComponentAtRootTag", std::move(params));
  }

  // Give the JS thread time to finish executing
  m_jsMessageThread.Load()->runOnQueueSync([]() {});
}

Mso::CntPtr<IReactInstanceInternal> MakeReactInstance(
    IReactHost &reactHost,
    ReactOptions &&options,
    Mso::Promise<void> &&whenCreated,
    Mso::Promise<void> &&whenLoaded,
    Mso::VoidFunctor &&updateUI) noexcept {
  return Mso::Make<ReactInstanceWin, IReactInstanceInternal>(
      reactHost, std::move(options), std::move(whenCreated), std::move(whenLoaded), std::move(updateUI));
}

#if defined(USE_V8)
std::string ReactInstanceWin::getApplicationLocalFolder() {
  auto local = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path();

  return Microsoft::Common::Unicode::Utf16ToUtf8(local.c_str(), local.size()) + "\\";
}
#endif

bool ReactInstanceWin::UseWebDebugger() const noexcept {
  return m_useWebDebugger;
}

bool ReactInstanceWin::UseFastRefresh() const noexcept {
  return m_isFastReloadEnabled;
}

bool ReactInstanceWin::UseDirectDebugger() const noexcept {
  return m_useDirectDebugger;
}

bool ReactInstanceWin::DebuggerBreakOnNextLine() const noexcept {
  return m_debuggerBreakOnNextLine;
}

uint16_t ReactInstanceWin::DebuggerPort() const noexcept {
  return m_options.DeveloperSettings.DebuggerPort;
}

std::string ReactInstanceWin::DebugBundlePath() const noexcept {
  return m_options.DeveloperSettings.SourceBundleName.empty() ? m_options.Identity
                                                              : m_options.DeveloperSettings.SourceBundleName;
}

std::string ReactInstanceWin::BundleRootPath() const noexcept {
  return m_options.BundleRootPath.empty() ? "ms-appx:///Bundle/" : m_options.BundleRootPath;
}

std::string ReactInstanceWin::SourceBundleHost() const noexcept {
  return m_options.DeveloperSettings.SourceBundleHost.empty() ? facebook::react::DevServerHelper::DefaultPackagerHost
                                                              : m_options.DeveloperSettings.SourceBundleHost;
}

uint16_t ReactInstanceWin::SourceBundlePort() const noexcept {
  return m_options.DeveloperSettings.SourceBundlePort ? m_options.DeveloperSettings.SourceBundlePort
                                                      : facebook::react::DevServerHelper::DefaultPackagerPort;
}

std::string ReactInstanceWin::JavaScriptBundleFile() const noexcept {
  return m_options.Identity;
}

bool ReactInstanceWin::UseDeveloperSupport() const noexcept {
  return m_options.UseDeveloperSupport();
}

Mso::React::IReactContext &ReactInstanceWin::GetReactContext() const noexcept {
  return *m_reactContext;
}

} // namespace Mso::React
