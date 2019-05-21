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
#include "network/message.hpp"
#include "network/service/call_context.hpp"
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

  static constexpr char const *LOGGING_NAME = "ServiceServerInterface";

  virtual ~ServiceServerInterface() = default;

  void Add(protocol_handler_type const &name,
           Protocol *                   protocol)  // TODO(issue 19): Rename to AddProtocol
  {
    LOG_STACK_TRACE_POINT;

    if (name < 1 || name > 255)
    {
      throw serializers::SerializableException(
          error::PROTOCOL_RANGE,
          byte_array_type(std::to_string(name) + " is out of protocol range."));
    }

    // TODO(issue 19): better reporting of errors
    if (members_[name] != nullptr)
    {
      throw serializers::SerializableException(error::PROTOCOL_EXISTS,
                                               byte_array_type("Protocol already exists. "));
    }

    members_[name] = protocol;
  }

protected:
  virtual bool DeliverResponse(connection_handle_type, network::message_type const &) = 0;

  bool PushProtocolRequest(connection_handle_type client, network::message_type const &msg,
                           CallContext const *context = nullptr)
  {
    LOG_STACK_TRACE_POINT;

    serializer_type             params(msg);
    service_classification_type type;
    params >> type;

    FETCH_LOG_DEBUG(LOGGING_NAME, "PushProtocolRequest type=", type);

    bool success = false;

    switch (type)
    {
    case SERVICE_FUNCTION_CALL:
      success = HandleRPCCallRequest(client, params, context);
      break;
    case SERVICE_SUBSCRIBE:
      success = HandleSubscribeRequest(client, params);
      break;
    case SERVICE_UNSUBSCRIBE:
      success = HandleUnsubscribeRequest(client, params);
      break;
    default:
      FETCH_LOG_WARN(LOGGING_NAME, "PushProtocolRequest type not recognised ", type);
      break;
    }

    return success;
  }

  bool HandleRPCCallRequest(connection_handle_type client, serializer_type params,
                            CallContext const *context = 0)
  {
    bool            ret = true;
    serializer_type result;
    PromiseCounter  id;

    try
    {
      LOG_STACK_TRACE_POINT;
      params >> id;
      FETCH_LOG_DEBUG(LOGGING_NAME, "HandleRPCCallRequest prom =", id);
      result << SERVICE_RESULT << id;
      ExecuteCall(result, client, params, context);
    }
    catch (serializers::SerializableException const &e)
    {
      LOG_STACK_TRACE_POINT;
      FETCH_LOG_ERROR(LOGGING_NAME, "Serialization error (Function Call): ", e.what());
      result = serializer_type();
      result << SERVICE_ERROR << id << e;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Service Server responding to call from ", client,
                    " data size=", result.tell());

    {
      LOG_STACK_TRACE_POINT;
      DeliverResponse(client, result.data());
    }
    return ret;
  }
  bool HandleSubscribeRequest(connection_handle_type client, serializer_type params)
  {
    bool                      ret = true;
    protocol_handler_type     protocol;
    feed_handler_type         feed;
    subscription_handler_type subid;

    try
    {
      LOG_STACK_TRACE_POINT;
      params >> protocol >> feed >> subid;
      auto &mod = *members_[protocol];

      mod.Subscribe(client, feed, subid);
    }
    catch (serializers::SerializableException const &e)
    {
      LOG_STACK_TRACE_POINT;
      FETCH_LOG_ERROR(LOGGING_NAME, "Serialization error (Subscribe): ", e.what());
      // result = serializer_type();
      // result << SERVICE_ERROR << id << e;
      throw e;  // TODO(ed): propagate error other other size
    }
    // DeliverResponse(client, result.data());
    return ret;
  }

  bool HandleUnsubscribeRequest(connection_handle_type client, serializer_type params)
  {
    bool                      ret = true;
    protocol_handler_type     protocol;
    feed_handler_type         feed;
    subscription_handler_type subid;

    try
    {
      LOG_STACK_TRACE_POINT;
      params >> protocol >> feed >> subid;
      auto &mod = *members_[protocol];

      mod.Unsubscribe(client, feed, subid);
    }
    catch (serializers::SerializableException const &e)
    {
      LOG_STACK_TRACE_POINT;
      FETCH_LOG_ERROR(LOGGING_NAME, "Serialization error (Unsubscribe): ", e.what());
      // result = serializer_type();
      // result << SERVICE_ERROR << id << e;
      throw e;  // TODO(ed): propagate error other other size
    }
    // DeliverResponse(client, result.data());
    return ret;
  }

  virtual void ConnectionDropped(connection_handle_type connection_handle)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "ConnectionDropped: ", connection_handle);
    for (int protocol_number = 0; protocol_number < 256; protocol_number++)
    {
      if (members_[protocol_number])
      {
        FETCH_LOG_WARN(LOGGING_NAME, "ConnectionDropped removing handler for protocol ",
                       protocol_number, " from connection handle ", connection_handle);
        members_[protocol_number]->ConnectionDropped(connection_handle);
        members_[protocol_number] = 0;
      }
    }
  }

private:
  void ExecuteCall(serializer_type &result, connection_handle_type const &connection_handle,
                   serializer_type params, CallContext const *context = 0)
  {
    LOG_STACK_TRACE_POINT;

    protocol_handler_type protocol_number;
    function_handler_type function_number;
    params >> protocol_number >> function_number;

    auto identifier = std::to_string(protocol_number) + ":" + std::to_string(function_number) +
                      "@" + std::to_string(connection_handle);

    FETCH_LOG_DEBUG(LOGGING_NAME, "ServerInterface::ExecuteCall " + identifier);

    auto protocol_pointer = members_[protocol_number];
    if (protocol_pointer == nullptr)
    {
      auto err = std::string("ServerInterface::ExecuteCall: Could not find protocol ") + identifier;
      FETCH_LOG_WARN(LOGGING_NAME, err);
      throw serializers::SerializableException(error::PROTOCOL_NOT_FOUND, err);
    }

    protocol_pointer->ApplyMiddleware(connection_handle, params.data());

    auto function = (*protocol_pointer)[function_number];

    FETCH_LOG_DEBUG(LOGGING_NAME, std::string("ServerInterface::ExecuteCall: ") + identifier +
                                      " expecting following signature " + function->signature());

    // If we need to add client id to function arguments
    try
    {

      CallableArgumentList extra_args;

      if (function->meta_data() & Callable::CLIENT_ID_ARG)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Adding connection_handle ID meta data to ", identifier);
        extra_args.PushArgument(&connection_handle);
      }
      if (function->meta_data() & Callable::CLIENT_CONTEXT_ARG)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Adding call context meta data to ", identifier);
        extra_args.PushArgument(&context);
      }

      if (!extra_args.empty())
      {
        (*function)(result, extra_args, params);
      }
      else
      {
        (*function)(result, params);
      }
    }
    catch (serializers::SerializableException const &e)
    {
      std::string new_explanation = e.explanation() + std::string(" (Function signature: ") +
                                    function->signature() + std::string(") (Identification: ") +
                                    identifier;

      FETCH_LOG_INFO(LOGGING_NAME, "EXCEPTION:", e.error_code(), new_explanation);

      throw serializers::SerializableException(e.error_code(), new_explanation);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "ServerInterface::ExecuteCall - ", ex.what(), " - ",
                      identifier);

      std::string new_explanation = ex.what() + std::string(") (Identification: ") + identifier;
      throw serializers::SerializableException(0, new_explanation);
    }
  }

  Protocol *members_[256] = {nullptr};  // TODO(issue 19): Not thread-safe
  friend class FeedSubscriptionManager;
};
}  // namespace service
}  // namespace fetch
