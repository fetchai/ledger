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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/containers/queue.hpp"
#include "core/mutex.hpp"
#include "network/management/network_manager.hpp"
#include "network/muddle/packet.hpp"
#include "network/tcp/tcp_client.hpp"

#include <atomic>
#include <deque>
#include <memory>

class ScopeClient : public std::enable_shared_from_this<ScopeClient>
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ByteArray      = fetch::byte_array::ByteArray;

  // Construction / Destruction
  ScopeClient();
  ScopeClient(ScopeClient const &) = delete;
  ScopeClient(ScopeClient &&)      = delete;
  ~ScopeClient();

  void Ping(ConstByteArray const &host, uint16_t port);

  // Operators
  ScopeClient &operator=(ScopeClient const &) = delete;
  ScopeClient &operator=(ScopeClient &&) = delete;

private:
  enum class State
  {
    NOT_CONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED,
    CONNECTION_CLOSED,
  };

  using NetworkManager = fetch::network::NetworkManager;
  using TCPClient      = fetch::network::TCPClient;
  using TcpClientPtr   = std::unique_ptr<TCPClient>;
  using MessageQueue   = fetch::core::MPSCQueue<ByteArray, 1024>;
  using SyncState      = std::atomic<State>;

  void CreateClient(ConstByteArray const &host, uint16_t port);
  bool WaitUntilConnected();

  template <typename T>
  void SendMessage(T const &packet);

  template <typename T>
  void RecvMessage(T &packet);

  // message queue
  NetworkManager manager_{"main", 1};
  TcpClientPtr   client_;

  MessageQueue messages_;
  SyncState    state_{State::NOT_CONNECTED};
};