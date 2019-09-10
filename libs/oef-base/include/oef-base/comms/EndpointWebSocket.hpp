#pragma once

#include "fetch_teams/ledger/logger.hpp"
#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/RingBuffer.hpp"

#include "boost/asio/ip/tcp.hpp"
#include "boost/asio/strand.hpp"
#include "boost/beast.hpp"
#include "boost/beast/core.hpp"

#include <memory>

template <typename TXType>
class EndpointWebSocket : public EndpointBase<TXType>,
                          public std::enable_shared_from_this<EndpointWebSocket<TXType>>
{
public:
  using EndpointBase<TXType>::state;
  using EndpointBase<TXType>::readBuffer;
  using EndpointBase<TXType>::sendBuffer;
  using typename EndpointBase<TXType>::message_type;
  using std::enable_shared_from_this<EndpointWebSocket<TXType>>::shared_from_this;

  using ConfigMap = typename EndpointBase<TXType>::ConfigMap;
  using Socket    = typename EndpointBase<TXType>::Socket;
  using Lock      = typename EndpointBase<TXType>::Lock;
  using StateType = typename EndpointBase<TXType>::StateType;
  using WebSocket = boost::beast::websocket::stream<Socket>;

  static constexpr char const *LOGGING_NAME = "EndpointWebSocket";

  explicit EndpointWebSocket(boost::asio::io_context &io_context, std::size_t sendBufferSize,
                             std::size_t readBufferSize, ConfigMap configMap);

  virtual ~EndpointWebSocket();

  EndpointWebSocket(const EndpointWebSocket &other) = delete;
  EndpointWebSocket &operator=(const EndpointWebSocket &other)  = delete;
  bool               operator==(const EndpointWebSocket &other) = delete;
  bool               operator<(const EndpointWebSocket &other)  = delete;

  virtual Socket &socket() override
  {
    return web_socket_.next_layer();
  }

  virtual void close() override;
  virtual void go() override;

protected:
  virtual void async_read(const std::size_t &bytes_needed) override;
  virtual void async_write() override;
  virtual bool is_eof(const boost::system::error_code &ec) const override;

  void async_read_at_least(const std::size_t &bytes_needed, std::size_t bytes_read,
                           std::vector<RingBuffer::mutable_buffer> &space,
                           std::shared_ptr<StateType>               my_state);

private:
  void on_accept(const boost::system::error_code &ec);

private:
  WebSocket                                                   web_socket_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
};
