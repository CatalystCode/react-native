// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off

#pragma once

#include <IWebSocketResource.h>

namespace Microsoft::React
{

class WinRTWebSocketResource : public IWebSocketResource
{
#pragma region IWebSocketResource

  /// <summary>
  /// <see cref="IWebSocketResource::Connect" />
  /// </summary>
  void Connect(const Protocols& protocols, const Options& options) override;

  /// <summary>
  /// <see cref="IWebSocketResource::Ping" />
  /// </summary>
  void Ping() override;

  /// <summary>
  /// <see cref="IWebSocketResource::Send" />
  /// </summary>
  void Send(const std::string& message) override;

  /// <summary>
  /// <see cref="IWebSocketResource::SendBinary" />
  /// </summary>
  void SendBinary(const std::string& base64String) override;

  /// <summary>
  /// <see cref="IWebSocketResource::Close" />
  /// </summary>
  void Close(CloseCode code, const std::string& reason) override;

  ReadyState GetReadyState() const override;

  /// <summary>
  /// <see cref="IWebSocketResource::SetOnConnect" />
  /// </summary>
  void SetOnConnect(std::function<void()>&& handler) override;

  /// <summary>
  /// <see cref="IWebSocketResource::SetOnPing" />
  /// </summary>
  void SetOnPing(std::function<void()>&& handler) override;

  /// <summary>
  /// <see cref="IWebSocketResource::SetOnSend" />
  /// </summary>
  void SetOnSend(std::function<void(std::size_t)>&& handler) override;

  /// <summary>
  /// <see cref="IWebSocketResource::SetOnMessage" />
  /// </summary>
  void SetOnMessage(std::function<void(std::size_t, const std::string&)>&& handler) override;

  /// <summary>
  /// <see cref="IWebSocketResource::SetOnClose" />
  /// </summary>
  void SetOnClose(std::function<void(CloseCode, const std::string&)>&& handler) override;

  /// <summary>
  /// <see cref="IWebSocketResource::SetOnError" />
  /// </summary>
  void SetOnError(std::function<void(Error&&)>&& handler) override;

#pragma endregion IWebSocketResource
};

} // namespace Microsoft::React
