// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include <cxxreact/CxxModule.h>
#include <cxxreact/CxxNativeModule.h>
#include <cxxreact/Instance.h>
#include <cxxreact/JSBigString.h>
#include <cxxreact/JSExecutor.h>
#include <cxxreact/ReactMarker.h>
#include <folly/json.h>
#include <jsi/jsi.h>
#include <jsiexecutor/jsireact/JSIExecutor.h>
#include <filesystem>
#include "OInstance.h"
#include "Unicode.h"

#include "Chakra/ChakraHelpers.h"
#include "Chakra/ChakraUtils.h"
#include "JSI/RuntimeHolder.h"

#include <cxxreact/MessageQueueThread.h>
#include <cxxreact/ModuleRegistry.h>

#include <Modules/ExceptionsManagerModule.h>
#include <Modules/PlatformConstantsModule.h>
#include <Modules/SourceCodeModule.h>
#include <Modules/StatusBarManagerModule.h>

#if (defined(_MSC_VER) && (defined(WINRT)))
#include <Utils/LocalBundleReader.h>
#endif

#include <BatchingMessageQueueThread.h>
#include <CreateModules.h>
#include <DevSettings.h>
#include <DevSupportManager.h>
#include <IReactRootView.h>
#include <RuntimeOptions.h>
#include <Shlwapi.h>
#include <WebSocketJSExecutorFactory.h>
#include "PackagerConnection.h"

#if defined(INCLUDE_HERMES)
#include "HermesRuntimeHolder.h"
#endif
#if defined(USE_V8)
#include "BaseScriptStoreImpl.h"
#include "V8JSIRuntimeHolder.h"
#endif
#include <ReactCommon/CallInvoker.h>
#include <ReactCommon/TurboModuleBinding.h>
#include "ChakraRuntimeHolder.h"

#include <tracing/tracing.h>
namespace fs = std::filesystem;

namespace {

#if (defined(_MSC_VER) && !defined(WINRT))

std::string GetJSBundleDirectory(
    const std::string &jsBundleBasePath,
    const std::string &jsBundleRelativePath) noexcept {
  // If there is a base path, use that to calculate the absolute path.
  if (jsBundleBasePath.length() > 0) {
    std::string jsBundleDirectory = jsBundleBasePath;
    if (jsBundleDirectory.back() != '\\')
      jsBundleDirectory += '\\';

    return jsBundleDirectory += jsBundleRelativePath;
  } else if (!PathIsRelativeA(jsBundleRelativePath.c_str())) {
    // If the given path is an absolute path, return it as-is
    return jsBundleRelativePath;
  }
  // Otherwise use the path of the executable file to construct the absolute
  // path.
  else {
    wchar_t modulePath[MAX_PATH];

    auto len = GetModuleFileNameW(nullptr, modulePath, _countof(modulePath));

    if (len == 0 || (len == _countof(modulePath) && GetLastError() == ERROR_INSUFFICIENT_BUFFER))
      return jsBundleRelativePath;

    // remove the trailing filename as we are interested in only the path
    auto succeeded = PathRemoveFileSpecW(modulePath);
    if (!succeeded)
      return jsBundleRelativePath;

    std::string jsBundlePath = Microsoft::Common::Unicode::Utf16ToUtf8(modulePath, wcslen(modulePath));
    if (!jsBundlePath.empty() && jsBundlePath.back() != '\\')
      jsBundlePath += '\\';

    return jsBundlePath += jsBundleRelativePath;
  }
}

std::string GetJSBundleFilePath(const std::string &jsBundleBasePath, const std::string &jsBundleRelativePath) {
  auto jsBundleFilePath = GetJSBundleDirectory(jsBundleBasePath, jsBundleRelativePath);

  // Usually, the module name: "module name" + "." + "platform name", for
  // example "lpc.win32". If we can not find the the bundle.js file under the
  // normal folder, we are trying to find it under the folder without the dot
  // (e.g. "lpcwin32"). VSO:1997035 remove this code after we have better name
  // convension
  if (PathFileExistsA((jsBundleFilePath + "\\bundle.js").c_str())) {
    jsBundleFilePath += "\\bundle.js";
  } else {
    // remove the dot only if is belongs to the bundle file name.
    size_t lastDotPosition = jsBundleFilePath.find_last_of('.');
    size_t bundleFilePosition = jsBundleFilePath.find_last_of('\\');
    if (lastDotPosition != std::string::npos &&
        (bundleFilePosition == std::string::npos || lastDotPosition > bundleFilePosition)) {
      jsBundleFilePath.erase(lastDotPosition, 1);
    }

    jsBundleFilePath += "\\bundle.js";

    // Back before we have base path plumbed through, we made win32 force a
    // seperate folder for each bundle by appending bundle.js Now that we have
    // base path, we can handle multiple SDXs with the same index name so we
    // should switch to using the same scheme as all the other platforms (and
    // the bundle server)
    // TODO: We should remove all the previous logic and use the same names as
    // the other platforms...
    if (!PathFileExistsA(jsBundleFilePath.c_str())) {
      jsBundleFilePath = GetJSBundleDirectory(jsBundleBasePath, jsBundleRelativePath) + ".bundle";
    }
  }

  return jsBundleFilePath;
}
#endif

} // namespace

using namespace facebook;

namespace facebook {
namespace react {

namespace {

class OJSIExecutorFactory : public JSExecutorFactory {
 public:
  std::unique_ptr<JSExecutor> createJSExecutor(
      std::shared_ptr<ExecutorDelegate> delegate,
      std::shared_ptr<MessageQueueThread> jsQueue) override {
    Logger logger;
    if (loggingHook_) {
      // TODO :: Ensure the logLevels are mapped properly.
      logger = [loggingHook = std::move(loggingHook_)](const std::string &message, unsigned int logLevel) {
        loggingHook(static_cast<RCTLogLevel>(logLevel), message.c_str());
      };
    } else {
      logger = [loggingHook = std::move(loggingHook_)](const std::string & /*message*/, unsigned int /*logLevel*/) {};
    }
    bindNativeLogger(*runtimeHolder_->getRuntime(), logger);

    auto turboModuleManager = std::make_shared<TurboModuleManager>(turboModuleRegistry_, jsCallInvoker_);

    // TODO: The binding here should also add the proxys that convert cxxmodules into turbomodules
    auto binding = [turboModuleManager](const std::string &name) -> std::shared_ptr<TurboModule> {
      return turboModuleManager->getModule(name);
    };

    TurboModuleBinding::install(*runtimeHolder_->getRuntime(), std::function(binding));

    // init TurboModule
    for (const auto &moduleName : turboModuleManager->getEagerInitModuleNames()) {
      turboModuleManager->getModule(moduleName);
    }

    return std::make_unique<JSIExecutor>(
        runtimeHolder_->getRuntime(),
        std::move(delegate),
        JSIExecutor::defaultTimeoutInvoker,
        [isProfiling = isProfilingEnabled_]([[maybe_unused]] jsi::Runtime &runtime) {
#ifdef ENABLE_JS_SYSTRACE_TO_ETW
          facebook::react::tracing::initializeJSHooks(runtime, isProfiling);
#endif
        });
  }

  OJSIExecutorFactory(
      std::shared_ptr<jsi::RuntimeHolderLazyInit> runtimeHolder,
      NativeLoggingHook loggingHook,
      std::shared_ptr<TurboModuleRegistry> turboModuleRegistry,
      bool isProfilingEnabled,
      std::shared_ptr<CallInvoker> jsCallInvoker) noexcept
      : runtimeHolder_{std::move(runtimeHolder)},
        loggingHook_{std::move(loggingHook)},
        turboModuleRegistry_{std::move(turboModuleRegistry)},
        jsCallInvoker_{std::move(jsCallInvoker)},
        isProfilingEnabled_{isProfilingEnabled} {}

 private:
  std::shared_ptr<jsi::RuntimeHolderLazyInit> runtimeHolder_;
  std::shared_ptr<TurboModuleRegistry> turboModuleRegistry_;
  std::shared_ptr<CallInvoker> jsCallInvoker_;
  NativeLoggingHook loggingHook_;
  bool isProfilingEnabled_;
};

} // namespace

void logMarker(const facebook::react::ReactMarker::ReactMarkerId /*id*/, const char * /*tag*/) {}

/*static*/ std::shared_ptr<InstanceImpl> InstanceImpl::MakeNoBundle(
    std::shared_ptr<Instance> &&instance,
    std::string &&jsBundleBasePath,
    std::vector<
        std::tuple<std::string, facebook::xplat::module::CxxModule::Provider, std::shared_ptr<MessageQueueThread>>>
        &&cxxModules,
    std::shared_ptr<TurboModuleRegistry> turboModuleRegistry,
    std::unique_ptr<InstanceCallback> &&callback,
    std::shared_ptr<MessageQueueThread> jsQueue,
    std::shared_ptr<MessageQueueThread> nativeQueue,
    std::shared_ptr<DevSettings> devSettings,
    std::shared_ptr<IDevSupportManager> devManager) noexcept {
  auto inner = std::shared_ptr<InstanceImpl>(new InstanceImpl(
      std::move(instance),
      std::move(jsBundleBasePath),
      std::move(cxxModules),
      std::move(turboModuleRegistry),
      std::move(callback),
      std::move(jsQueue),
      std::move(nativeQueue),
      std::move(devSettings),
      std::move(devManager)));

  inner->RegisterForReloadIfNecessary();

  return inner;
}

/*static*/ std::shared_ptr<InstanceImpl> InstanceImpl::MakeAndLoadBundle(
    std::shared_ptr<Instance> &&instance,
    std::string &&jsBundleBasePath,
    std::string &&jsBundleRelativePath,
    std::vector<
        std::tuple<std::string, facebook::xplat::module::CxxModule::Provider, std::shared_ptr<MessageQueueThread>>>
        &&cxxModules,
    std::shared_ptr<TurboModuleRegistry> turboModuleRegistry,
    std::unique_ptr<InstanceCallback> &&callback,
    std::shared_ptr<MessageQueueThread> jsQueue,
    std::shared_ptr<MessageQueueThread> nativeQueue,
    std::shared_ptr<DevSettings> devSettings,
    std::shared_ptr<IDevSupportManager> devManager) noexcept {
  auto inner = std::shared_ptr<InstanceImpl>(new InstanceImpl(
      std::move(instance),
      std::move(jsBundleBasePath),
      std::move(cxxModules),
      std::move(turboModuleRegistry),
      std::move(callback),
      std::move(jsQueue),
      std::move(nativeQueue),
      std::move(devSettings),
      std::move(devManager)));

  inner->loadBundle(std::move(jsBundleRelativePath));
  inner->RegisterForReloadIfNecessary();

  return inner;
}

void InstanceImpl::SetInError() noexcept {
  m_isInError = true;
}

InstanceImpl::InstanceImpl(
    std::shared_ptr<Instance> &&instance,
    std::string &&jsBundleBasePath,
    std::vector<
        std::tuple<std::string, facebook::xplat::module::CxxModule::Provider, std::shared_ptr<MessageQueueThread>>>
        &&cxxModules,
    std::shared_ptr<TurboModuleRegistry> turboModuleRegistry,
    std::unique_ptr<InstanceCallback> &&callback,
    std::shared_ptr<MessageQueueThread> jsQueue,
    std::shared_ptr<MessageQueueThread> nativeQueue,
    std::shared_ptr<DevSettings> devSettings,
    std::shared_ptr<IDevSupportManager> devManager)
    : m_turboModuleRegistry(std::move(turboModuleRegistry)),
      m_jsThread(std::move(jsQueue)),
      m_nativeQueue(nativeQueue),
      m_jsBundleBasePath(std::move(jsBundleBasePath)),
      m_devSettings(std::move(devSettings)),
      m_devManager(std::move(devManager)),
      m_innerInstance(std::move(instance)) {
  // Temp set the logmarker here
  facebook::react::ReactMarker::logTaggedMarker = logMarker;

#ifdef ENABLE_ETW_TRACING
  // TODO :: Find a better place to initialize ETW once per process.
  facebook::react::tracing::initializeETW();
#endif

  if (m_devSettings->useDirectDebugger && !m_devSettings->useWebDebugger) {
    m_devManager->StartInspector(m_devSettings->sourceBundleHost, m_devSettings->sourceBundlePort);
  }

  // Default (common) NativeModules
  auto modules = GetDefaultNativeModules(nativeQueue);

  // Add app provided modules.
  for (auto &cxxModule : cxxModules) {
    modules.push_back(std::make_unique<CxxNativeModule>(
        m_innerInstance, move(std::get<0>(cxxModule)), move(std::get<1>(cxxModule)), move(std::get<2>(cxxModule))));
  }
  m_moduleRegistry = std::make_shared<facebook::react::ModuleRegistry>(std::move(modules));

  // Choose JSExecutor
  std::shared_ptr<JSExecutorFactory> jsef;
  if (m_devSettings->useWebDebugger) {
    try {
      auto jseFunc = m_devManager->LoadJavaScriptInProxyMode(*m_devSettings, [weakthis = weak_from_this()]() {
        if (auto strongThis = weakthis.lock()) {
          strongThis->SetInError();
        }
      });

      if ((jseFunc == nullptr) || m_isInError) {
        m_devSettings->errorCallback("Failed to create JavaScript Executor.");
        return;
      }

      jsef = std::make_shared<WebSocketJSExecutorFactory>(std::move(jseFunc));
    } catch (std::exception &e) {
      m_devSettings->errorCallback(e.what());
      return;
    }
  } else {
    if (m_devSettings->useFastRefresh || m_devSettings->liveReloadCallback) {
      Microsoft::ReactNative::PackagerConnection::CreateOrReusePackagerConnection(*m_devSettings);
    }

    // If the consumer gives us a JSI runtime, then  use it.
    if (m_devSettings->jsiRuntimeHolder) {
      assert(m_devSettings->jsiEngineOverride == JSIEngineOverride::Default);
      jsef = std::make_shared<OJSIExecutorFactory>(
          m_devSettings->jsiRuntimeHolder,
          m_devSettings->loggingCallback,
          m_turboModuleRegistry,
          !m_devSettings->useFastRefresh,
          m_innerInstance->getJSCallInvoker());
    } else {
      assert(m_devSettings->jsiEngineOverride != JSIEngineOverride::Default);
      switch (m_devSettings->jsiEngineOverride) {
        case JSIEngineOverride::Hermes:
#if defined(INCLUDE_HERMES)
          m_devSettings->jsiRuntimeHolder = std::make_shared<HermesRuntimeHolder>(m_devSettings, m_jsThread);
          m_devSettings->inlineSourceMap = false;
          break;
#else
          assert(false); // Hermes is not available in this build, fallthrough
          [[fallthrough]];
#endif
        case JSIEngineOverride::V8: {
#if defined(USE_V8)
          std::unique_ptr<facebook::jsi::ScriptStore> scriptStore = nullptr;
          std::unique_ptr<facebook::jsi::PreparedScriptStore> preparedScriptStore = nullptr;

          char tempPath[MAX_PATH];
          if (GetTempPathA(MAX_PATH, tempPath)) {
            preparedScriptStore = std::make_unique<facebook::react::BasePreparedScriptStoreImpl>(tempPath);
          }

          m_devSettings->jsiRuntimeHolder = std::make_shared<facebook::react::V8JSIRuntimeHolder>(
              m_devSettings, m_jsThread, std::move(scriptStore), std::move(preparedScriptStore));
          break;
#else
          assert(false); // V8 is not available in this build, fallthrough
          [[fallthrough]];
#endif
        }
        case JSIEngineOverride::Chakra:
          // Applies only to ChakraCore-linked binaries.
          Microsoft::React::SetRuntimeOptionBool("JSI.ForceSystemChakra", true);
          m_devSettings->jsiRuntimeHolder =
              std::make_shared<Microsoft::JSI::ChakraRuntimeHolder>(m_devSettings, m_jsThread, nullptr, nullptr);
          break;
        case JSIEngineOverride::ChakraCore:
        default: // TODO: Add other engines once supported
          m_devSettings->jsiRuntimeHolder =
              std::make_shared<Microsoft::JSI::ChakraRuntimeHolder>(m_devSettings, m_jsThread, nullptr, nullptr);
          break;
      }
      jsef = std::make_shared<OJSIExecutorFactory>(
          m_devSettings->jsiRuntimeHolder,
          m_devSettings->loggingCallback,
          m_turboModuleRegistry,
          !m_devSettings->useFastRefresh,
          m_innerInstance->getJSCallInvoker());
    }
  }

  m_innerInstance->initializeBridge(std::move(callback), jsef, m_jsThread, m_moduleRegistry);

  // All JSI runtimes do support host objects and hence the native modules
  // proxy.
  const bool isNativeModulesProxyAvailable = ((m_devSettings->jsiRuntimeHolder != nullptr) ||
                                              (m_devSettings->jsiEngineOverride != JSIEngineOverride::Default)) &&
      !m_devSettings->useWebDebugger;
  if (!isNativeModulesProxyAvailable) {
    folly::dynamic configArray = folly::dynamic::array;
    for (auto const &moduleName : m_moduleRegistry->moduleNames()) {
      auto moduleConfig = m_moduleRegistry->getConfig(moduleName);
      configArray.push_back(moduleConfig ? std::move(moduleConfig->config) : nullptr);
    }

    folly::dynamic configs = folly::dynamic::object("remoteModuleConfig", std::move(configArray));
    m_innerInstance->setGlobalVariable(
        "__fbBatchedBridgeConfig", std::make_unique<JSBigStdString>(folly::toJson(configs)));
  }
}

void InstanceImpl::loadBundle(std::string &&jsBundleRelativePath) {
  loadBundleInternal(std::move(jsBundleRelativePath), /*synchronously:*/ false);
}

void InstanceImpl::loadBundleSync(std::string &&jsBundleRelativePath) {
  loadBundleInternal(std::move(jsBundleRelativePath), /*synchronously:*/ true);
}

void InstanceImpl::loadBundleInternal(std::string &&jsBundleRelativePath, bool synchronously) {
  try {
    if (m_devSettings->useWebDebugger || m_devSettings->liveReloadCallback != nullptr ||
        m_devSettings->useFastRefresh) {
      // First attempt to get download the Js locally, to catch any bundling
      // errors before attempting to load the actual script.

      auto [jsBundleString, success] = Microsoft::ReactNative::GetJavaScriptFromServer(
          m_devSettings->sourceBundleHost,
          m_devSettings->sourceBundlePort,
          m_devSettings->debugBundlePath.empty() ? jsBundleRelativePath : m_devSettings->debugBundlePath,
          m_devSettings->platformName,
          true /* dev */,
          m_devSettings->useFastRefresh,
          m_devSettings->inlineSourceMap);

      if (!success) {
        m_devManager->UpdateBundleStatus(false, -1);
        m_devSettings->errorCallback(jsBundleString);
        return;
      }

      int64_t currentTimeInMilliSeconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono ::system_clock::now().time_since_epoch())
              .count();
      m_devManager->UpdateBundleStatus(true, currentTimeInMilliSeconds);

      auto bundleUrl = DevServerHelper::get_BundleUrl(
          m_devSettings->sourceBundleHost,
          m_devSettings->sourceBundlePort,
          m_devSettings->debugBundlePath.empty() ? jsBundleRelativePath : m_devSettings->debugBundlePath,
          m_devSettings->platformName,
          /*dev*/ true,
          /*hot*/ false,
          m_devSettings->inlineSourceMap);

      // Remote debug executor loads script from a Uri, rather than taking the actual bundle string
      m_innerInstance->loadScriptFromString(
          std::make_unique<const JSBigStdString>(m_devSettings->useWebDebugger ? bundleUrl : jsBundleString),
          bundleUrl,
          synchronously);
    } else {
#if (defined(_MSC_VER) && !defined(WINRT))
      auto fullBundleFilePath = GetJSBundleFilePath(m_jsBundleBasePath, jsBundleRelativePath);

      // If fullBundleFilePath exists, load User bundle.
      // Otherwise all bundles (User and Platform) are loaded through
      // platformBundles.
      if (PathFileExistsA(fullBundleFilePath.c_str())) {
#if defined(_CHAKRACORE_H_)
        auto bundleString = FileMappingBigString::fromPath(fullBundleFilePath);
#else
        auto bundleString = JSBigFileString::fromPath(fullBundleFilePath);
#endif
        m_innerInstance->loadScriptFromString(std::move(bundleString), std::move(fullBundleFilePath), synchronously);
      }

#else
      std::string bundlePath = (fs::path(m_devSettings->bundleRootPath) / (jsBundleRelativePath + ".bundle")).string();

      auto bundleString = std::make_unique<::Microsoft::ReactNative::StorageFileBigString>(bundlePath);
      m_innerInstance->loadScriptFromString(std::move(bundleString), jsBundleRelativePath, synchronously);
#endif
    }
#if defined(_CHAKRACORE_H_)
  } catch (const facebook::react::ChakraJSException &e) {
    m_devSettings->errorCallback(std::string{e.what()} + "\r\n" + e.getStack());
#endif
  } catch (const std::exception &e) {
    m_devSettings->errorCallback(e.what());
  } catch (const winrt::hresult_error &hrerr) {
    std::stringstream ss;
    ss << "[" << std::hex << std::showbase << std::setw(8) << static_cast<uint32_t>(hrerr.code()) << "] "
       << winrt::to_string(hrerr.message());

    m_devSettings->errorCallback(std::move(ss.str()));
  }
}

InstanceImpl::~InstanceImpl() {
  m_devManager->StopInspector();
  m_nativeQueue->quitSynchronous();
}

std::vector<std::unique_ptr<NativeModule>> InstanceImpl::GetDefaultNativeModules(
    std::shared_ptr<MessageQueueThread> nativeQueue) {
  std::vector<std::unique_ptr<NativeModule>> modules;

  modules.push_back(std::make_unique<CxxNativeModule>(
      m_innerInstance,
      "WebSocketModule",
      [nativeQueue]() -> std::unique_ptr<xplat::module::CxxModule> {
        return Microsoft::React::CreateWebSocketModule();
      },
      nativeQueue));

// TODO: This is not included for UWP because we have a different module which
// is added later. However, this one is designed
//  so that we can base a UWP version on it. We need to do that but is not high
//  priority.
#if (defined(_MSC_VER) && !defined(WINRT))
  modules.push_back(std::make_unique<CxxNativeModule>(
      m_innerInstance,
      "Timing",
      [nativeQueue]() -> std::unique_ptr<xplat::module::CxxModule> { return react::CreateTimingModule(nativeQueue); },
      nativeQueue));
#endif

  // TODO - Encapsulate this in a helpers, and make sure callers add it to their
  // list
  std::string bundleUrl = (m_devSettings->useWebDebugger || m_devSettings->liveReloadCallback)
      ? DevServerHelper::get_BundleUrl(
            m_devSettings->sourceBundleHost,
            m_devSettings->sourceBundlePort,
            m_devSettings->debugBundlePath,
            m_devSettings->platformName,
            true /*dev*/,
            m_devSettings->useFastRefresh,
            m_devSettings->inlineSourceMap)
      : std::string();
  modules.push_back(std::make_unique<CxxNativeModule>(
      m_innerInstance,
      facebook::react::SourceCodeModule::Name,
      [bundleUrl]() -> std::unique_ptr<xplat::module::CxxModule> {
        return std::make_unique<SourceCodeModule>(bundleUrl);
      },
      nativeQueue));

  modules.push_back(std::make_unique<CxxNativeModule>(
      m_innerInstance,
      "ExceptionsManager",
      [redboxHandler = m_devSettings->redboxHandler]() mutable {
        return std::make_unique<ExceptionsManagerModule>(redboxHandler);
      },
      nativeQueue));

  modules.push_back(std::make_unique<CxxNativeModule>(
      m_innerInstance,
      PlatformConstantsModule::Name,
      []() { return std::make_unique<PlatformConstantsModule>(); },
      nativeQueue));

  modules.push_back(std::make_unique<CxxNativeModule>(
      m_innerInstance,
      StatusBarManagerModule::Name,
      []() { return std::make_unique<StatusBarManagerModule>(); },
      nativeQueue));

  return modules;
}

void InstanceImpl::RegisterForReloadIfNecessary() noexcept {
  // setup polling for live reload
  if (!m_isInError && !m_devSettings->useFastRefresh && m_devSettings->liveReloadCallback != nullptr) {
    m_devManager->StartPollingLiveReload(
        m_devSettings->sourceBundleHost, m_devSettings->sourceBundlePort, m_devSettings->liveReloadCallback);
  }
}

void InstanceImpl::DispatchEvent(int64_t viewTag, std::string eventName, folly::dynamic &&eventData) {
  if (m_isInError) {
    return;
  }

  folly::dynamic params = folly::dynamic::array(viewTag, eventName, std::move(eventData));
  m_innerInstance->callJSFunction("RCTEventEmitter", "receiveEvent", std::move(params));
}

void InstanceImpl::invokeCallback(const int64_t callbackId, folly::dynamic &&params) {
  if (m_isInError) {
    return;
  }

  m_innerInstance->callJSCallback(callbackId, std::move(params));
}

} // namespace react
} // namespace facebook
