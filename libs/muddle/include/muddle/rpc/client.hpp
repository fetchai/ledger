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

#include "core/mutex.hpp"
#include "muddle/address.hpp"
#include "muddle/muddle_endpoint.hpp"
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
  using Address       = muddle::Address;
  using ProtocolId    = service::ProtocolHandlerType;
  using FunctionId    = service::FunctionHandlerType;
  using Serializer    = service::SerializerType;
  using Promise       = service::Promise;
  using Handler       = std::function<void(Promise)>;
  using SharedHandler = std::shared_ptr<Handler>;
  using WeakHandler   = std::weak_ptr<Handler>;

  static constexpr char const *LOGGING_NAME = "MuddleRpcClient";

  // Construction / Destruction
  Client(std::string name, MuddleEndpoint &endpoint, uint16_t service, uint16_t channel);
  Client(Client const &) = delete;
  Client(Client &&)      = delete;
  ~Client() override     = default;

  template <typename... Args>
  Promise CallSpecificAddress(Address const &address, ProtocolId const &protocol,
                              FunctionId const &function, Args &&... args)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Service Client Calling ", protocol, ":", function);

    // create and register the promise to this callback
    Promise prom = service::MakePromise(protocol, function);
    AddPromise(prom);

    // determine the required serial size
    serializers::SizeCounter counter;
    counter << service::SERVICE_FUNCTION_CALL << prom->id();
    service::PackCall(counter, protocol, function, args...);

    // pack the mesage into a buffer
    service::SerializerType params;
    params.Reserve(counter.size());
    params << service::SERVICE_FUNCTION_CALL << prom->id();
    service::PackCall(params, protocol, function, std::forward<Args>(args)...);

    FETCH_LOG_TRACE(LOGGING_NAME, "Registering promise ", prom->id(), " with ", protocol, ':',
                    function, " (call) ", &promises_);

    if (!DeliverRequest(address, params.data()))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Call to ", protocol, ":", function, " prom=", prom->id(),
                     " failed!");

      prom->Fail(serializers::SerializableException(
          service::error::COULD_NOT_DELIVER,
          byte_array::ConstByteArray("Could not deliver request in " __FILE__)));

      RemovePromise(prom->id());
    }

    return prom;
  }

  // Operators
  Client &operator=(Client const &) = delete;
  Client &operator=(Client &&) = delete;

protected:
  bool DeliverRequest(muddle::Address const &address, network::MessageBuffer const &data);

private:
  using Flag            = std::atomic<bool>;
  using PromiseQueue    = std::list<MuddleEndpoint::Response>;
  using SubscriptionPtr = MuddleEndpoint::SubscriptionPtr;

  void OnMessage(Packet const &packet, Address const &last_hop);

  static std::size_t const NUM_THREADS = 1;

  std::string const name_;
  MuddleEndpoint &  endpoint_;
  SubscriptionPtr   subscription_;
  NetworkId const   network_id_;
  uint16_t const    service_;
  uint16_t const    channel_;
};

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
