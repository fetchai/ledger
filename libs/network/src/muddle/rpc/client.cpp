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

#include "network/muddle/rpc/client.hpp"
#include "core/threading.hpp"

#include <list>

namespace fetch {
namespace muddle {
namespace rpc {


Client::Client(std::string name, MuddleEndpoint &endpoint, Address address, uint16_t service,
               uint16_t channel)
  : name_(std::move(name))
  , endpoint_(endpoint)
  , address_(std::move(address))
  , network_id_(endpoint.network_id())
  , service_(service)
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

  // start the background thread processs
  running_           = true;
  background_thread_ = std::thread{&Client::BackgroundWorker, this};
}

Client::Client(std::string name, MuddleEndpoint &endpoint, uint16_t service, uint16_t channel)
  : Client(std::move(name), endpoint, Address{}, service, channel)
{
}

Client::~Client()
{

  FETCH_LOG_WARN(LOGGING_NAME, "Client teardown...");
  // clear that handler
  handler_.reset();

  FETCH_LOG_WARN(LOGGING_NAME, "Handler reset, stopping threadpool");
  running_ = false;
  promise_queue_cv_.notify_all();
  background_thread_.join();
  background_thread_ = std::thread{};
  FETCH_LOG_WARN(LOGGING_NAME, "Threadpool stopped, client destructor end");
}

bool Client::DeliverRequest(network::message_type const &data)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Please send this packet to the server  ", service_, ",", channel_);

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

    // Add this new wrapping promise to the execution queue
    {
      std::unique_lock<std::mutex> lk(promise_queue_lock_);
      promise_queue_.emplace_back(std::move(promise));
      promise_queue_cv_.notify_one();
    }

    return true;
  }
  catch (std::exception &e)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Erk! Exception in endpoint_.Exchange ", "@prom=", ident, " ",
                    e.what());
    throw e;
  }
}

void Client::BackgroundWorker()
{
  using PromiseState = service::PromiseState;

  SetThreadName(name_);

  std::list<MuddleEndpoint::Response> pending_promises;

  while (running_)
  {
    // attempt to extract and promises from the queue
    {
      std::unique_lock<std::mutex> lk(promise_queue_lock_);
      if (!promise_queue_.empty())
      {
        pending_promises.splice(pending_promises.end(), promise_queue_);
      }
    }

    // evaluate the promises in the queue
    auto it = pending_promises.begin();
    while (it != pending_promises.end())
    {
      // evaulate the state of the current promise
      PromiseState const state = it->GetState();

      if (service::PromiseState::WAITING != state)
      {
        // erase the promise from the queue
        it = pending_promises.erase(it);
      }
      else
      {
        // advance to the next iterator
        ++it;
      }
    }
    if (pending_promises.empty())
    {
      std::unique_lock<std::mutex> lk(promise_queue_lock_);
      promise_queue_cv_.wait(lk);
    }
  }
}

}  // namespace rpc
}  // namespace muddle
}  // namespace fetch
