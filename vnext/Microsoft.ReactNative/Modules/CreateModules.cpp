// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <CreateModules.h>
#include <Modules/WebSocketModule.h>
#include <Modules/WebSocketModuleUwp.h>
#include <WinRTWebSocketResource.h>

// React Native
#include <cxxreact/CxxModule.h>

// Windows API
#include <winrt/Windows.Security.Cryptography.Certificates.h>

namespace Microsoft::React {

std::shared_ptr<IWebSocketResource> IWebSocketResource::Make(std::string &&urlString) {
  // TODO: use QuirkSettings
  std::vector<winrt::Windows::Security::Cryptography::Certificates::ChainValidationResult> certExceptions;
  return std::make_shared<WinRTWebSocketResource>(std::move(urlString), std::move(certExceptions));
}

std::unique_ptr<facebook::xplat::module::CxxModule> CreateWebSocketModule() noexcept {
  // TODO: use QuirkSettings
  if (false) {
    return std::make_unique<react::uwp::LegacyWebSocketModule>();
  }
  return std::make_unique<WebSocketModule>();
}

} // namespace Microsoft::React
