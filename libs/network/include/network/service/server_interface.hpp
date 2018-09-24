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
#include "network/message.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/message_types.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "network/service/types.hpp"

namespace fetch {
namespace service {

class ServiceServerInterface
{
public:
  using connection_handle_type = network::AbstractConnection::connection_handle_type;
  using byte_array_type        = byte_array::ConstByteArray;

  virtual ~ServiceServerInterface()
  {}

  void Add(protocol_handler_type const &name,
           Protocol *                   protocol)  // TODO(issue 19): Rename to AddProtocol
  {
    LOG_STACK_TRACE_POINT;

    // TODO(issue 19): better reporting of errors
    if (members_[name] != nullptr)
    {
      throw serializers::SerializableException(error::PROTOCOL_EXISTS,
                                               byte_array_type("Protocol already exists. "));
    }

    members_[name] = protocol;

    for (auto &feed : protocol->feeds())
    {
      feed->AttachToService(this);
    }
  }

protected:
  virtual bool DeliverResponse(connection_handle_type, network::message_type const &) = 0;

  bool PushProtocolRequest(connection_handle_type client, network::message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    bool            ret = false;
    serializer_type params(msg);

    service_classification_type type;
    params >> type;

    if (type == SERVICE_FUNCTION_CALL)  // TODO(issue 19): change to switch
    {
      ret = true;
      serializer_type               result;
      Promise::promise_counter_type id;

      try
      {
        params >> id;
        result << SERVICE_RESULT << id;

        ExecuteCall(result, client, params);
      }
      catch (serializers::SerializableException const &e)
      {
        fetch::logger.Error("Serialization error (Function Call): ", e.what());
        result = serializer_type();
        result << SERVICE_ERROR << id << e;
      }

      fetch::logger.Debug("Service Server responding to call from ", client);
      DeliverResponse(client, result.data());
    }
    else if (type == SERVICE_SUBSCRIBE)
    {
      ret = true;
      protocol_handler_type     protocol;
      feed_handler_type         feed;
      subscription_handler_type subid;

      try
      {
        params >> protocol >> feed >> subid;
        auto &mod = *members_[protocol];

        mod.Subscribe(client, feed, subid);
      }
      catch (serializers::SerializableException const &e)
      {
        fetch::logger.Error("Serialization error (Subscribe): ", e.what());
        // result = serializer_type();
        // result << SERVICE_ERROR << id << e;
        throw e;  // TODO(tfr): propagate error other other size
      }
      // DeliverResponse(client, result.data());
    }
    else if (type == SERVICE_UNSUBSCRIBE)
    {
      ret = true;
      protocol_handler_type     protocol;
      feed_handler_type         feed;
      subscription_handler_type subid;

      try
      {
        params >> protocol >> feed >> subid;
        auto &mod = *members_[protocol];

        mod.Unsubscribe(client, feed, subid);
      }
      catch (serializers::SerializableException const &e)
      {
        fetch::logger.Error("Serialization error (Unsubscribe): ", e.what());
        // result = serializer_type();
        // result << SERVICE_ERROR << id << e;
        throw e;  // TODO(tfr): propagate error other other size
      }
      // DeliverResponse(client, result.data());
    }

    return ret;
  }

private:
  void ExecuteCall(serializer_type &result, connection_handle_type const &client,
                   serializer_type params)
  {
    //    LOG_STACK_TRACE_POINT;
    LOG_STACK_TRACE_POINT;

    protocol_handler_type protocol;
    function_handler_type function;
    params >> protocol >> function;
    fetch::logger.Debug("Service Server processing call ", protocol, ":", function, " from ",
                        client);

    if (members_[protocol] == nullptr)
    {
      throw serializers::SerializableException(error::PROTOCOL_NOT_FOUND,
                                               byte_array_type("Could not find protocol: "));
    }

    auto &mod = *members_[protocol];

    mod.ApplyMiddleware(client, params.data());

    auto &fnc = mod[function];
    fetch::logger.Debug("Expecting following signature: ", fnc.signature());

    // If we need to add client id to function arguments
    try
    {
      if (fnc.meta_data() & Callable::CLIENT_ID_ARG)
      {
        fetch::logger.Debug("Adding client ID meta data to ", protocol, ":", function);
        CallableArgumentList extra_args;
        extra_args.PushArgument(&client);
        fnc(result, extra_args, params);
        return;
      }

      fnc(result, params);
      return;
    }
    catch (serializers::SerializableException const &e)
    {
      std::string new_explanation = e.explanation() + std::string(" (Function signature: ") +
                                    fnc.signature() + std::string(")");
      serializers::SerializableException e2(e.error_code(), new_explanation);
      throw e2;
    }
  }

  Protocol *members_[256] = {nullptr};  // TODO(issue 19): Not thread-safe
  friend class FeedSubscriptionManager;
};
}  // namespace service
}  // namespace fetch
