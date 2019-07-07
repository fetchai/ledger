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

#include "core/mutex.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/service/client_interface.hpp"
#include "network/service/promise.hpp"
#include "network/service/types.hpp"

#include <atomic>
#include <list>
#include <memory>
#include <thread>
#include <utility>

namespace fetch {
namespace muddle {
namespace rpc {

class Client : protected service::ServiceClientInterface
{
public:
  using Address       = MuddleEndpoint::Address;
  using ProtocolId    = service::protocol_handler_type;
  using FunctionId    = service::function_handler_type;
  using Serializer    = service::serializer_type;
  using Promise       = service::Promise;
  using ThreadPool    = network::ThreadPool;
  using Handler       = std::function<void(Promise)>;
  using SharedHandler = std::shared_ptr<Handler>;
  using WeakHandler   = std::weak_ptr<Handler>;

  static constexpr char const *LOGGING_NAME = "MuddleRpcClient";

  // Construction / Destruction
  Client(std::string name, MuddleEndpoint &endpoint, Address address, uint16_t service,
         uint16_t channel);
  Client(std::string name, MuddleEndpoint &endpoint, uint16_t service, uint16_t channel);
  Client(Client const &) = delete;
  Client(Client &&)      = delete;
  ~Client() override;

  template <typename... Args>
  Promise CallSpecificAddress(Address const &address, ProtocolId const &protocol,
                              FunctionId const &function, Args &&... args)
  {
    FETCH_LOCK(call_mutex_);
    LOG_STACK_TRACE_POINT;
    // update the target address
    address_ = address;

    // execute the call
    return Call(network_id_.value(), protocol, function, std::forward<Args>(args)...);
  }

  // Operators
  Client &operator=(Client const &) = delete;
  Client &operator=(Client &&) = delete;

protected:
  bool DeliverRequest(network::message_type const &data) override;

private:
  using Flag         = std::atomic<bool>;
  using Mutex        = fetch::mutex::Mutex;
  using PromiseQueue = std::list<MuddleEndpoint::Response>;

  static std::size_t const NUM_THREADS = 1;

  std::string const name_;
  MuddleEndpoint &  endpoint_;
  Address           address_;
  NetworkId const   network_id_;
  uint16_t const    service_;
  uint16_t const    channel_;

  SharedHandler handler_;

  std::mutex call_mutex_; // priority 1

  PromiseQueue            promise_queue_;
  std::mutex              promise_queue_lock_; // priority 2
  std::condition_variable promise_queue_cv_;

  std::thread background_thread_;
  Flag        running_{false};

  void BackgroundWorker();
};

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
