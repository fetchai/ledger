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

#include <memory>
#include <utility>

#include "network/details/thread_pool.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/service/client_interface.hpp"
#include "network/service/promise.hpp"
#include "network/service/types.hpp"

namespace fetch {
namespace muddle {
namespace rpc {

class Client : public service::ServiceClientInterface
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
  using NetworkId     = MuddleEndpoint::NetworkId;

  static constexpr char const *LOGGING_NAME = "MuddleRpcClient";

  Client(MuddleEndpoint &endpoint, Address address, uint16_t service, uint16_t channel)
    : endpoint_(endpoint)
    , address_(std::move(address))
    , service_(service)
    , network_id_(endpoint.network_id())
    , channel_(channel)
  {
    handler_ = std::make_shared<Handler>([this](Promise promise) {
      LOG_STACK_TRACE_POINT;

      FETCH_LOG_DEBUG(LOGGING_NAME, "Handling an inner promise ", promise->id());
      try
      {
        ProcessServerMessage(promise->value());
      }
      catch (std::exception &ex)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Client::ProcessServerMessage EX ", ex.what());
      }
    });

    thread_pool_->Start();
  }

  ~Client() override
  {

    FETCH_LOG_WARN(LOGGING_NAME, "Client teardown...");
    // clear that handler
    handler_.reset();

    FETCH_LOG_WARN(LOGGING_NAME, "Handler reset, stopping threadpool");
    thread_pool_->Stop();
    FETCH_LOG_WARN(LOGGING_NAME, "Threadpool stopped, client destructor end");
  }

  template <typename... Args>
  Promise CallSpecificAddress(Address const &address, ProtocolId const &protocol,
                              FunctionId const &function, Args &&... args)
  {
    // update the target address
    address_ = address;

    // execute the call
    return Call(network_id_, protocol, function, std::forward<Args>(args)...);
  }

protected:
  bool DeliverRequest(network::message_type const &data) override
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Please send this packet to the server  ", service_, ",",
                    channel_);

    unsigned long long int ident = 0;

    try
    {
      // signal to the networking that an exchange is requested
      auto promise = endpoint_.Exchange(address_, service_, channel_, data);
      ident        = promise.id();

      FETCH_LOG_DEBUG(LOGGING_NAME, "Sent this packet to the server  ", service_, ",", channel_,
                      "@prom=", promise.id(), " response size=", data.size());

      // establish the correct course of action when
      WeakHandler handler = handler_;
      promise.WithHandlers()
          .Then([handler, promise]() {
            LOG_STACK_TRACE_POINT;

            FETCH_LOG_DEBUG(LOGGING_NAME, "Got the response to our question...",
                            "@prom=", promise.id());
            auto callback = handler.lock();
            if (callback)
            {
              (*callback)(promise.GetInnerPromise());
            }
          })
          .Catch([promise]() {
            LOG_STACK_TRACE_POINT;

            // TODO(EJF): This is actually a bug since the RPC promise implementation doesn't have a
            // callback process
            FETCH_LOG_DEBUG(LOGGING_NAME, "Exchange promise failed", "@prom=", promise.id());
          });

      // TODO(EJF): Chained promises would remove the requirement for this
      thread_pool_->Post([promise]() {
        LOG_STACK_TRACE_POINT;
        promise.Wait();
      });

      return true;  //?
    }
    catch (std::exception &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Erk! Exception in endpoint_.Exchange ", "@prom=", ident, " ",
                      e.what());
      throw e;
    }
  }

private:
  MuddleEndpoint &endpoint_;
  Address         address_;
  uint16_t const  service_;
  NetworkId       network_id_;
  uint16_t const  channel_;
  ThreadPool      thread_pool_ = network::MakeThreadPool(10, "rpc::Client");
  SharedHandler   handler_;
};

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
