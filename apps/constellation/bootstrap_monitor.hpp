#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

class BootstrapMonitor
{
public:
  using UriList = Constellation::UriList;
  using Identity = crypto::Identity;

  BootstrapMonitor(Identity const &identity, uint16_t port, uint32_t network_id)
    : network_id_(network_id), port_(port), identity_(identity)
  {}

  bool Start(UriList &peers);
  void Stop();

  std::string const &external_address() const { return external_address_; }

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
