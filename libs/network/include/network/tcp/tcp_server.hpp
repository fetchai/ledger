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

#include "network/fetch_asio.hpp"
#include "network/generics/atomic_inflight_counter.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/management/network_manager.hpp"
#include "network/message.hpp"
#include "network/tcp/abstract_server.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

namespace fetch {
namespace network {

class AbstractConnectionRegister;
class ClientManager;

/*
 * Handle TCP connections. Spawn new ClientConnections on connect and adds
 * them to the client manager. This can then be used for communication with
 * the rest of Fetch
 *
 */
class TCPServer : public AbstractNetworkServer
{
public:
  using connection_handle_type = typename AbstractConnection::connection_handle_type;
  using network_manager_type   = NetworkManager;
  using acceptor_type          = asio::ip::tcp::tcp::acceptor;
  using mutex_type             = std::mutex;

  static constexpr char const *LOGGING_NAME = "TCPServer";

  struct Request
  {
    connection_handle_type handle;
    message_type           message;
  };

  TCPServer(uint16_t const &port, network_manager_type const &network_manager);
  ~TCPServer() override;

  // Start will block until the server has started
  virtual void Start();
  virtual void Stop();

  void PushRequest(connection_handle_type client, message_type const &msg) override;

  void Broadcast(message_type const &msg);
  bool Send(connection_handle_type const &client, message_type const &msg);

  bool has_requests();

  Request Top();
  void    Pop();

  std::string GetAddress(connection_handle_type const &client);

  template <typename X>
  void SetConnectionRegister(X &reg)
  {
    connection_register_ = reg.pointer();
  }

  void SetConnectionRegister(std::weak_ptr<AbstractConnectionRegister> const &reg)
  {
    connection_register_ = reg;
  }

  uint16_t port()
  {
    return port_;
  };

private:
  using InFlightCounter = AtomicInFlightCounter<network::AtomicCounterName::TCP_PORT_STARTUP>;

  void Accept(std::shared_ptr<asio::ip::tcp::tcp::acceptor> acceptor);

  network_manager_type                      network_manager_;
  uint16_t                                  port_;
  std::deque<Request>                       requests_;
  mutex_type                                request_mutex_;
  std::weak_ptr<AbstractConnectionRegister> connection_register_;
  std::shared_ptr<ClientManager>            manager_;
  std::weak_ptr<acceptor_type>              acceptor_;
  std::mutex                                start_mutex_;

  // Use this class to keep track of whether we are ready to accept connections
  InFlightCounter counter_;
};
}  // namespace network
}  // namespace fetch
