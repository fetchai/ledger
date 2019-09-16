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

#include "core/serializers/counter.hpp"
#include "core/serializers/serializable_exception.hpp"
#include "network/message.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/message_types.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "network/service/types.hpp"

#include <cstdint>
#include <cstdio>
#include <list>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace service {

class ServiceClientInterface
{
  class Subscription;

  using subscriptions_type = std::unordered_map<SubscriptionHandlerType, Subscription>;

public:
  static constexpr char const *LOGGING_NAME = "ServiceClientInterface";

  // Construction / Destruction
  ServiceClientInterface();
  virtual ~ServiceClientInterface() = default;

  template <typename... arguments>
  Promise Call(uint32_t /*network_id*/, ProtocolHandlerType const &protocol,
               FunctionHandlerType const &function, arguments &&... args)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Service Client Calling ", protocol, ":", function);

    Promise prom = MakePromise(protocol, function);

    SerializerType params;

    serializers::SizeCounter counter;
    counter << SERVICE_FUNCTION_CALL << prom->id();

    PackCall(counter, protocol, function, args...);

    params.Reserve(counter.size());

    params << SERVICE_FUNCTION_CALL << prom->id();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Registering promise ", prom->id(), " with ", protocol, ':',
                    function, " (call)", &promises_);

    AddPromise(prom);

    PackCall(params, protocol, function, std::forward<arguments>(args)...);

    if (!DeliverRequest(params.data()))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Call to ", protocol, ":", function, " prom=", prom->id(),
                     " failed!");

      prom->Fail(serializers::SerializableException(
          error::COULD_NOT_DELIVER,
          byte_array::ConstByteArray("Could not deliver request in " __FILE__)));

      RemovePromise(prom->id());
    }

    return prom;
  }

  Promise CallWithPackedArguments(ProtocolHandlerType const &  protocol,
                                  FunctionHandlerType const &  function,
                                  byte_array::ByteArray const &args);

  /// @name Subscriptions
  /// @{
  SubscriptionHandlerType Subscribe(ProtocolHandlerType const &protocol,
                                    FeedHandlerType const &feed, AbstractCallable *callback);
  void                    Unsubscribe(SubscriptionHandlerType id);
  /// @}

protected:
  virtual bool DeliverRequest(network::MessageType const &request) = 0;

  bool ProcessServerMessage(network::MessageType const &msg);
  void ProcessRPCResult(network::MessageType const &msg, service::SerializerType &params);

private:
  SubscriptionHandlerType CreateSubscription(ProtocolHandlerType const &protocol,
                                             FeedHandlerType const &feed, AbstractCallable *cb);

  class Subscription
  {
  public:
    Subscription()
    {
      protocol = 0;
      feed     = 0;
      callback = nullptr;
    }
    Subscription(ProtocolHandlerType protocol, FeedHandlerType feed, AbstractCallable *callback)
    {
      this->protocol = protocol;
      this->feed     = feed;
      this->callback = callback;
    }

    Subscription(const Subscription &) = default;
    Subscription(Subscription &&)      = default;
    Subscription &operator=(const Subscription &) = default;
    Subscription &operator=(Subscription &&) = default;

    std::string summarise()
    {
      char  buffer[1000];
      char *p = buffer;
      p += std::sprintf(p, " Subscription protocol=%d handler=%d callback=%p ", int(protocol),
                        int(feed), ((void *)(callback)));
      return std::string(buffer);
    }

    ProtocolHandlerType protocol = 0;
    FeedHandlerType     feed     = 0;
    AbstractCallable *  callback = nullptr;
  };

  void    AddPromise(Promise const &promise);
  Promise LookupPromise(PromiseCounter id);
  Promise ExtractPromise(PromiseCounter id);
  void    RemovePromise(PromiseCounter id);

  subscriptions_type                 subscriptions_;
  std::list<SubscriptionHandlerType> cancelled_subscriptions_;
  Mutex                              subscription_mutex_;
  SubscriptionHandlerType            subscription_index_counter_;

  std::map<PromiseCounter, Promise> promises_;
  Mutex                             promises_mutex_;
};
}  // namespace service
}  // namespace fetch
