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
#include "core/mutex.hpp"
#include "crypto/identity.hpp"
#include "http/json_client.hpp"
#include "network/fetch_asio.hpp"

#include "constellation.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace fetch {

/**
 * The bootstrap monitor is a simple placeholder implementation for a client to the bootstrap
 * server. It has two main functions namely:
 *
 * - The collection of an initial set of peers to attempt to connect to initially
 * - A periodic phone home in order to update the cached set of peer connections.
 */
class BootstrapMonitor
{
public:
  using UriList  = Constellation::UriList;
  using Identity = crypto::Identity;

  // Construction / Destruction
  BootstrapMonitor(Identity const &identity, uint16_t p2p_port, uint32_t network_id,
                   std::string token, std::string host_name)
    : network_id_(network_id)
    , port_(p2p_port)
    , identity_(identity)
    , token_(std::move(token))
    , host_name_(std::move(host_name))
  {}

  BootstrapMonitor(BootstrapMonitor const &) = delete;
  BootstrapMonitor(BootstrapMonitor &&)      = delete;

  ~BootstrapMonitor()
  {
    Stop();
  }

  bool Start(UriList &peers);
  void Stop();

  std::string const &external_address() const
  {
    return external_address_;
  }
  std::string const &interface_address() const
  {
    return external_address_;
  }

  // Operators
  BootstrapMonitor &operator=(BootstrapMonitor const &) = delete;
  BootstrapMonitor &operator=(BootstrapMonitor &&) = delete;

private:
  using IoService      = asio::io_service;
  using Resolver       = asio::ip::tcp::resolver;
  using Socket         = asio::ip::tcp::socket;
  using ThreadPtr      = std::unique_ptr<std::thread>;
  using Buffer         = byte_array::ByteArray;
  using Flag           = std::atomic<bool>;
  using Mutex          = mutex::Mutex;
  using LockGuard      = std::lock_guard<Mutex>;
  using ConstByteArray = byte_array::ConstByteArray;

  uint32_t const network_id_;
  uint16_t const port_;
  Identity const identity_;
  std::string    external_address_;
  std::string    token_;
  std::string    host_name_;

  Flag      running_{false};
  ThreadPtr monitor_thread_;
  Mutex     io_mutex_{__LINE__, __FILE__};
  IoService io_service_;
  Resolver  resolver_{io_service_};
  Socket    socket_{io_service_};
  Buffer    buffer_;

  bool UpdateExternalAddress();

  bool RequestPeerList(UriList &peers);
  bool RegisterNode();
  bool NotifyNode();

  void ThreadEntryPoint();
};

}  // namespace fetch
