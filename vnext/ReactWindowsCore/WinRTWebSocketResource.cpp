// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// clang-format off
#include "WinRTWebSocketResource.h"

#include <winrt/Windows.Security.Cryptography.h>
#include <Unicode.h>
#include <Utilities.h>

// Standard Library
#include <future>

using Microsoft::Common::Unicode::Utf8ToUtf16;
using Microsoft::Common::Unicode::Utf16ToUtf8;
using Microsoft::Common::Utilities::CheckedReinterpretCast;

using std::function;
using std::size_t;
using std::string;
using std::vector;

using winrt::fire_and_forget;
using winrt::hresult;
using winrt::hresult_error;
using winrt::resume_background;
using winrt::resume_on_signal;
using winrt::Windows::Foundation::IAsyncAction;
using winrt::Windows::Foundation::Uri;
using winrt::Windows::Networking::Sockets::IWebSocket;
using winrt::Windows::Networking::Sockets::MessageWebSocket;
using winrt::Windows::Networking::Sockets::MessageWebSocketMessageReceivedEventArgs;
using winrt::Windows::Networking::Sockets::SocketMessageType;
using winrt::Windows::Networking::Sockets::WebSocketClosedEventArgs;
using winrt::Windows::Security::Cryptography::Certificates::ChainValidationResult;
using winrt::Windows::Security::Cryptography::CryptographicBuffer;
using winrt::Windows::Storage::Streams::DataReader;
using winrt::Windows::Storage::Streams::DataWriter;
using winrt::Windows::Storage::Streams::UnicodeEncoding;

namespace Microsoft::React
{
WinRTWebSocketResource::WinRTWebSocketResource(Uri&& uri, vector<ChainValidationResult> certExeptions)
  : m_uri{ std::move(uri) }
{
  m_socket.MessageReceived({ this, &WinRTWebSocketResource::OnMessageReceived });
  m_socket.Closed({ this, &WinRTWebSocketResource::OnClosed });

  for (auto certException : certExeptions)
  {
    m_socket.Control().IgnorableServerCertificateErrors().Append(certException);
  }
}

WinRTWebSocketResource::WinRTWebSocketResource(const string& urlString, vector<ChainValidationResult> certExeptions)
  : WinRTWebSocketResource(Uri{ Utf8ToUtf16(urlString) }, certExeptions)
{
}

WinRTWebSocketResource::~WinRTWebSocketResource() /*override*/
{
  Synchronize();
}

#pragma region Private members

IAsyncAction WinRTWebSocketResource::PerformConnect()
{
  try
  {
    co_await resume_background();

    co_await m_socket.ConnectAsync(m_uri);

    m_readyState = ReadyState::Open;
    if (m_connectHandler)
    {
      m_connectHandler();
    }
  }
  catch (hresult_error const& e)
  {
    m_readyState = ReadyState::Closed;
    if (m_errorHandler)
    {
      m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Connection });
    }
  }

  SetEvent(m_connectPerformed.get());
  m_connectPerformedPromise.set_value();
  m_connectRequested = false;
}

fire_and_forget WinRTWebSocketResource::PerformPing()
{
  try
  {
    co_await resume_background();

    co_await resume_on_signal(m_connectPerformed.get());

    if (m_readyState != ReadyState::Open)
    {
      co_return;
    }

    m_socket.Control().MessageType(SocketMessageType::Utf8);

    string s{};
    winrt::array_view<const uint8_t> arr(
      CheckedReinterpretCast<const uint8_t*>(s.c_str()),
      CheckedReinterpretCast<const uint8_t*>(s.c_str()) + s.length());
    m_writer.WriteBytes(arr);

    co_await m_writer.StoreAsync();

    if (m_pingHandler)
    {
      m_pingHandler();
    }
  }
  catch (hresult_error const& e)
  {
    if (m_errorHandler)
    {
      m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Ping });
    }
  }
}

fire_and_forget WinRTWebSocketResource::PerformWrite()
{
  if (m_writeQueue.empty())
  {
    co_return;
  }

  try
  {
    co_await resume_background();

    co_await resume_on_signal(m_connectPerformed.get());

    if (m_readyState != ReadyState::Open)
    {
      co_return;
    }

    size_t length;
    std::pair<string, bool> front;
    bool popped = m_writeQueue.try_pop(front);
    if (!popped)
    {
      throw hresult_error(E_FAIL, L"Could not retrieve outgoing message.");
    }

    auto [message, isBinary] = std::move(front);

    if (isBinary)
    {
      m_socket.Control().MessageType(SocketMessageType::Binary);

      auto buffer = CryptographicBuffer::DecodeFromBase64String(Utf8ToUtf16(std::move(message)));
      length = buffer.Length();
      m_writer.WriteBuffer(buffer);
    }
    else
    {
      m_socket.Control().MessageType(SocketMessageType::Utf8);

      //TODO: Use char_t instead of uint8_t?
      length = message.size();
      winrt::array_view<const uint8_t> arr(
        CheckedReinterpretCast<const uint8_t*>(message.c_str()),
        CheckedReinterpretCast<const uint8_t*>(message.c_str()) + message.length());
      m_writer.WriteBytes(arr);
    }

    co_await m_writer.StoreAsync();

    if (m_writeHandler)
    {
      m_writeHandler(length);
    }

    if (!m_writeQueue.empty())
    {
      PerformWrite();
    }
  }
  catch (hresult_error const& e)
  {
    if (m_errorHandler)
    {
      m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Ping });
    }
  }
}

fire_and_forget WinRTWebSocketResource::PerformClose()
{
  co_await resume_background();

  co_await resume_on_signal(m_connectPerformed.get());

  m_socket.Close(static_cast<uint16_t>(m_closeCode), Utf8ToUtf16(m_closeReason));

  m_readyState = ReadyState::Closed;

  co_return;
}

void WinRTWebSocketResource::OnMessageReceived(IWebSocket const& sender, MessageWebSocketMessageReceivedEventArgs const& args)
{
  try
  {
    string response;
    DataReader reader = args.GetDataReader();
    auto len = reader.UnconsumedBufferLength();
    if (args.MessageType() == SocketMessageType::Utf8)
    {
      reader.UnicodeEncoding(UnicodeEncoding::Utf8);
      vector<uint8_t> data(len);
      reader.ReadBytes(data);

      response = string(CheckedReinterpretCast<char*>(data.data()), data.size());
    }
    else
    {
      auto buffer = reader.ReadBuffer(len);
      winrt::hstring data = CryptographicBuffer::EncodeToBase64String(buffer);

      response = Utf16ToUtf8(std::wstring_view(data));
    }

    if (m_readHandler)
    {
      m_readHandler(response.length(), response);
    }
  }
  catch (hresult_error const& e)
  {
    if (m_errorHandler)
    {
      m_errorHandler({ Utf16ToUtf8(e.message()), ErrorType::Receive });
    }
  }
}

void WinRTWebSocketResource::OnClosed(IWebSocket const& sender, WebSocketClosedEventArgs const& args)
{
  if (m_closeHandler)
  {
    //TODO: Parameterize (pass via member variables?)
    m_closeHandler(CloseCode::Normal, "Closing");
  }
}

void WinRTWebSocketResource::Synchronize()
{
  // Ensure sequence of other operations
  if (m_connectRequested)
  {
    m_connectPerformedPromise.get_future().wait();
  }
}

#pragma endregion Private members

#pragma region IWebSocketResource

void WinRTWebSocketResource::Connect(const Protocols& protocols, const Options& options)
{
  m_readyState = ReadyState::Connecting;

  for (const auto& header : options)
  {
    m_socket.SetRequestHeader(header.first, Utf8ToUtf16(header.second));
  }

  winrt::Windows::Foundation::Collections::IVector<winrt::hstring> supportedProtocols =
    m_socket.Control().SupportedProtocols();
  for (const auto& protocol : protocols)
  {
    supportedProtocols.Append(Utf8ToUtf16(protocol));
  }

  m_connectRequested = true;
  PerformConnect();
}

void WinRTWebSocketResource::Ping()
{
  PerformPing();
}

void WinRTWebSocketResource::Send(const string& message)
{
  m_writeQueue.push({ message, false });

  PerformWrite();
}

void WinRTWebSocketResource::SendBinary(const string& base64String)
{
  m_writeQueue.push({ base64String, false });

  PerformWrite();
}

void WinRTWebSocketResource::Close(CloseCode code, const string& reason)
{
  Synchronize();

  m_closeCode = code;
  m_closeReason = reason;

  PerformClose();
}

IWebSocketResource::ReadyState WinRTWebSocketResource::GetReadyState() const
{
  return m_readyState;
}

void WinRTWebSocketResource::SetOnConnect(function<void()>&& handler)
{
  m_connectHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnPing(function<void()>&& handler)
{
  m_pingHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnSend(function<void(size_t)>&& handler)
{
  m_writeHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnMessage(function<void(size_t, const string&)>&& handler)
{
  m_readHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnClose(function<void(CloseCode, const string&)>&& handler)
{
  m_closeHandler = std::move(handler);
}

void WinRTWebSocketResource::SetOnError(function<void(Error&&)>&& handler)
{
  m_errorHandler = std::move(handler);
}

#pragma endregion IWebSocketResource

}// namespace Microsoft::React
