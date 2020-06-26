// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "DevServerHelper.h"

#include <functional>
#include <memory>
#include <string>

namespace facebook {
namespace react {

struct DevSettings;

struct IDevSupportManager {
  virtual JSECreator LoadJavaScriptInProxyMode(const DevSettings &settings) = 0;
  virtual std::string GetJavaScriptFromServer(
      const std::string &sourceBundleHost,
      const uint16_t sourceBundlePort,
      const std::string &jsBundleName,
      const std::string &platform) = 0;
  virtual void StartPollingLiveReload(
      const std::string &sourceBundleHost,
      const uint16_t sourceBundlePort,
      std::function<void()> onChangeCallback) = 0;
  virtual void StopPollingLiveReload() = 0;
  virtual bool HasException() = 0;
};

std::shared_ptr<IDevSupportManager> CreateDevSupportManager();

} // namespace react
} // namespace facebook
