// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <boost/beast/websocket.hpp>
#include <thread>
#include <queue>
#include "IWebSocket.h"
#include "Utils.h"

#include <boost/beast/core/detail/type_traits.hpp>

namespace facebook {
namespace react {

template<typename Protocol, typename Socket, typename Resolver>
class BaseWebSocket : public IWebSocket
{
  std::function<void()> m_connectHandler;
  std::function<void()> m_pingHandler;
  std::function<void(std::size_t)> m_writeHandler;
  std::function<void(std::size_t, const std::string&)> m_readHandler;
  std::function<void(CloseCode, const std::string&)> m_closeHandler;

  Url m_url;
  ReadyState m_readyState { ReadyState::Connecting };
  boost::beast::multi_buffer m_bufferIn;
  std::thread m_contextThread;

  ///
  // Must be modified exclusively from the context thread.
  ///
  std::queue<std::pair<std::string, bool>> m_writeRequests;

  std::atomic_size_t m_pingRequests { 0 };
  CloseCode m_closeCodeRequest { CloseCode::None };
  std::string m_closeReasonRequest;

  // Internal status flags.
  std::atomic_bool m_handshakePerformed { false };
  std::atomic_bool m_closeRequested { false };
  std::atomic_bool m_closeInProgress { false };
  std::atomic_bool m_pingInProgress { false };
  std::atomic_bool m_writeInProgress { false };

  void EnqueueWrite(const std::string& message, bool binary);
  void PerformWrite();
  void PerformRead();
  void PerformPing();
  void PerformClose();

  ///
  // Synchronizes the context thread and allows the io_context to stop dispatching tasks.
  ///
  void Stop();

  boost::beast::websocket::close_code ToBeastCloseCode(IWebSocket::CloseCode closeCode);

protected:
  std::function<void(Error&&)> m_errorHandler;

  boost::asio::io_context m_context;
  std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> m_workGuard;
  std::unique_ptr<boost::beast::websocket::stream<Socket>> m_stream;

  BaseWebSocket(Url&& url);

  ~BaseWebSocket() override;

  virtual void Handshake(const IWebSocket::Options& options);

public:
  #pragma region IWebSocket members

  void Connect(const Protocols& protocols, const Options& options) override;
  void Ping() override;
  void Send(const std::string& message) override;
  void SendBinary(const std::string& base64String) override;
  void Close(CloseCode code, const std::string& reason) override;

  ReadyState GetReadyState() const override;

  void SetOnConnect(std::function<void()>&& handler) override;
  void SetOnPing(std::function<void()>&& handler) override;
  void SetOnSend(std::function<void(std::size_t)>&& handler) override;
  void SetOnMessage(std::function<void(std::size_t, const std::string&)>&& handler) override;
  void SetOnClose(std::function<void(CloseCode, const std::string&)>&& handler) override;
  void SetOnError(std::function<void(Error&&)>&& handler) override;

  #pragma endregion
};

template<typename Protocol, typename Socket = boost::asio::basic_stream_socket<Protocol>, typename Resolver = boost::asio::ip::basic_resolver<Protocol>>
class WebSocket : public BaseWebSocket<Protocol, Socket, Resolver>
{
public:
  WebSocket(Url&& url);
};

template<typename Protocol, typename Socket, typename Resolver = boost::asio::ip::basic_resolver<Protocol>>
class SecureWebSocket : public BaseWebSocket<Protocol, Socket, Resolver>
{
  #pragma region BaseWebSocket overrides

  void Handshake(const IWebSocket::Options& options) override;

  #pragma endregion

public:
  SecureWebSocket(Url&& url);
};

namespace test
{

struct IExecutor {};

struct IWebSocketTransport
{
  typedef boost::asio::ip::basic_endpoint<IWebSocketTransport> endpoint;

  typedef boost::asio::basic_stream_socket<IWebSocketTransport> socket;

  int dummy;
};

//TODO: Likely delete. Hard to mock boost::asio::basic_stream_socket
struct IWebSocketStream
{
  using executor_type = IExecutor;

  //boost::asio::async_result
  //<
  //  boost::asio::decay
  //  <
  //    //std::function<void(boost::system::error_code, std::size_t)>
  //    std::function<void(const boost::system::error_code&, std::size_t)>
  //  >::type,
  //  void(boost::system::error_code, std::size_t)
  //>
  BOOST_ASIO_INITFN_RESULT_TYPE(std::function<void(boost::system::error_code, std::size_t)>, void(boost::system::error_code, std::size_t))
  async_read_some(boost::beast::multi_buffer& buffers, std::function<void(const boost::system::error_code&, std::size_t)>&& handler);

  //boost::asio::async_result
  //<
  //  boost::asio::decay
  //  <
  //    //std::function<void(boost::system::error_code, std::size_t)>
  //    std::function<void(const boost::system::error_code&, std::size_t)>
  //  >::type,
  //  void(boost::system::error_code, std::size_t)
  //>
  BOOST_ASIO_INITFN_RESULT_TYPE(std::function<void(boost::system::error_code, std::size_t)>, void(boost::system::error_code, std::size_t))
  async_write_some(boost::beast::multi_buffer& buffers, std::function<void(const boost::system::error_code&, std::size_t)>&& handler);

  void* get_executor();
};

struct IResolver
{
  using results_type = std::vector<std::string>;

  void async_resolve(std::string host, std::string port, std::function<void(const boost::system::error_code&, results_type)>);
};

class TestResolver : public IResolver
{
public:
  TestResolver(const boost::asio::io_context& context);
};

} // namespace facebook::react::test

} } // namespace facebook::react

namespace boost {
namespace beast {
namespace websocket {

void async_teardown(
  role_type role,
  facebook::react::test::IWebSocketTransport& socket,
  std::function<void(error_code const& error)>&& handler)
{
}

} } } // namespace boost::beast::websocket

namespace facebook {
namespace react {
namespace test {

//class TestWebSocket : public BaseWebSocket<IWebSocketTransport, boost::asio::basic_stream_socket<IWebSocketTransport>, TestResolver>
//{
//};

} } }
