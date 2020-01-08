#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "logging/logging.hpp"
#include "oef-base/comms/EndpointBase.hpp"
#include "oef-base/comms/RingBuffer.hpp"

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
  //  using WebSocket = boost::beast::websocket::stream<Socket>;

  static constexpr char const *LOGGING_NAME = "EndpointWebSocket";

  explicit EndpointWebSocket(asio::io_context &io_context, std::size_t sendBufferSize,
                             std::size_t readBufferSize, ConfigMap configMap);

  ~EndpointWebSocket() override;

  EndpointWebSocket(const EndpointWebSocket &other) = delete;
  EndpointWebSocket &operator=(const EndpointWebSocket &other)  = delete;
  bool               operator==(const EndpointWebSocket &other) = delete;
  bool               operator<(const EndpointWebSocket &other)  = delete;

  /*
  virtual Socket &socket() override
  {
    return web_socket_.next_layer();
  }
*/

  void close() override;
  void go() override;

protected:
  void async_read(const std::size_t &bytes_needed) override;
  void async_write() override;
  bool is_eof(std::error_code const &ec) const override;

  void async_read_at_least(const std::size_t &bytes_needed, std::size_t bytes_read,
                           std::vector<RingBuffer::mutable_buffer> &space,
                           std::shared_ptr<StateType>               my_state);

private:
  void on_accept(std::error_code const &ec);

private:
  // WebSocket                                     web_socket_;
  //  asio::strand<asio::io_context::executor_type> strand_;
};
