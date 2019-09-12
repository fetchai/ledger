#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/macros.hpp"
#include "logging/logging.hpp"
#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/RingBuffer.hpp"

//#include "boost/beast.hpp"  // TODO: Get rid of beast
//#include "boost/beast/core.hpp"
#include "network/fetch_asio.hpp"

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
  // TODO: Fix  using WebSocket = beast::websocket::stream<Socket>;

  static constexpr char const *LOGGING_NAME = "EndpointWebSocket";

  explicit EndpointWebSocket(asio::io_context &io_context, std::size_t sendBufferSize,
                             std::size_t readBufferSize, ConfigMap configMap);

  virtual ~EndpointWebSocket();

  EndpointWebSocket(EndpointWebSocket const &other) = delete;
  EndpointWebSocket &operator=(EndpointWebSocket const &other)  = delete;
  bool               operator==(EndpointWebSocket const &other) = delete;
  bool               operator<(EndpointWebSocket const &other)  = delete;

  virtual Socket &socket() override
  {
    Socket *ptr = nullptr;
    FETCH_UNUSED(ptr);
    return *ptr;  // TODO: web_socket_.next_layer();
  }

  virtual void close() override;
  virtual void go() override;

protected:
  virtual void async_read(std::size_t const &bytes_needed) override;
  virtual void async_write() override;
  virtual bool is_eof(std::error_code const &ec) const override;

  void async_read_at_least(std::size_t const &bytes_needed, std::size_t bytes_read,
                           std::vector<RingBuffer::mutable_buffer> &space,
                           std::shared_ptr<StateType>               my_state);

private:
  void on_accept(std::error_code const &ec);

private:
  /*
  TODO FIX:
  WebSocket                                     web_socket_;
  asio::strand<asio::io_context::executor_type> strand_;
  */
};
