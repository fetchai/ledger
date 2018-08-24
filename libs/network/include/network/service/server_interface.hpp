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
#include "network/generics/atomic_state_machine.hpp"
#include "core/mutex.hpp"

namespace fetch {
namespace service {

class ServiceServerInterface
{
public:
  using connection_handle_type = network::AbstractConnection::connection_handle_type;
  using byte_array_type        = byte_array::ConstByteArray;
  using mutex_type = fetch::mutex::Mutex;
  using lock_type = std::lock_guard<mutex_type>;

  enum {
    EMPTY,
    ADDED,
    MESSAGES,
    SUBSCRIBED,
    DEAD,
  };
  network::AtomicStateMachine protocol_states_;
  static constexpr char const *LOGGING_NAME = "ServiceServerInterface";
  ServiceServerInterface()
  {
    protocol_states_
      .Allow(ADDED, EMPTY)
      .Allow(MESSAGES, ADDED)
      .Allow(SUBSCRIBED, ADDED)

      .Allow(SUBSCRIBED, MESSAGES)
      .Allow(MESSAGES, SUBSCRIBED)

      .Allow(DEAD, EMPTY)
      .Allow(DEAD, ADDED)
      .Allow(DEAD, MESSAGES)
      .Allow(DEAD, SUBSCRIBED)
      ;

    auto msg = std::string("ServerInterface::ServiceServerInterface START ")
      + "   "
      + DescribeProtocolSystem();

    FETCH_LOG_WARN(LOGGING_NAME, msg);
  }

  virtual ~ServiceServerInterface()
  {
    protocol_states_.Set(DEAD);
    RemoveAllProtocols(0);
  }

  void Add(protocol_handler_type const &name,
           Protocol *                   protocol)  // TODO: Rename to AddProtocol
  {
    LOG_STACK_TRACE_POINT;

    // TODO: (`HUT`) : better reporting of errors
    if (members_[name] != nullptr)
    {
      throw serializers::SerializableException(error::PROTOCOL_EXISTS,
                                               byte_array_type("Protocol already exists. "));
    }

    fetch::logger.Warn("Assigning protocol ", int(name));
    int foo;

    members_[name] = protocol;
    for (auto &feed : protocol->feeds())
    {
      feed->AttachToService(this);
    }

    try
    {
      foo = protocol_states_.Get();
      protocol_states_.Set(ADDED);
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("ServerInterface::Add BAD_STATE_TRANSITION: ", ex.what());
      throw;
    }

    auto msg = std::string("ServerInterface::Add: Add protocol ")
      + std::to_string(int(name))
      + "   "
      + DescribeProtocolSystem();

    FETCH_LOG_WARN(LOGGING_NAME, msg);
  }

protected:
  virtual bool DeliverResponse(connection_handle_type, network::message_type const &) = 0;

  bool PushProtocolRequest(connection_handle_type client, network::message_type const &msg)
  {
    try
    {
      protocol_states_.Set(MESSAGES);
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("ServerInterface::PushProtocolRequest BAD_STATE_TRANSITION: ",
                          ex.what(),
                          " client=",
                          client,
                          "  ",
                          IdentifyProtocolSystem());
      return true;
    }

    LOG_STACK_TRACE_POINT;
    serializer_type params(msg);
    service_classification_type type;
    params >> type;

    switch(type)
    {
    case SERVICE_FUNCTION_CALL:
      return HandleRPCCallRequest(client, params);
    case SERVICE_SUBSCRIBE:
      return HandleSubscribeRequest(client, params);
    case SERVICE_UNSUBSCRIBE:
      return HandleUnsubscribeRequest(client, params);
    default:
      FETCH_LOG_ERROR(LOGGING_NAME,"PushProtocolRequest BAD TYPE:", type);
      return false;
    }
  }

  bool HandleRPCCallRequest(connection_handle_type client, serializer_type params)
  {
    LOG_STACK_TRACE_POINT;
    bool ret = true;
    serializer_type               result;
    Promise::promise_counter_type id;

    try
    {
      protocol_states_.Set(MESSAGES);
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("ServerInterface::ExecuteCallHandleRPCCallRequest BAD_STATE_TRANSITION: ",
                          ex.what(),
                          " client=",
                          client,
                          "  ",
                          IdentifyProtocolSystem());
      throw;
    }

    try
    {
      LOG_STACK_TRACE_POINT;
      params >> id;
      result << SERVICE_RESULT << id;
    }
    catch (serializers::SerializableException const &e)
    {
      LOG_STACK_TRACE_POINT;
      fetch::logger.Error("Serialization error (Function Call): ", e.what());
      result = serializer_type();
      result << SERVICE_ERROR << id << e;
    }

    try
    {
      ExecuteCall(result, client, params);
    }
    catch (serializers::SerializableException const &e)
    {
      LOG_STACK_TRACE_POINT;
      fetch::logger.Error("Serialization error (Function Call): ", e.what());
      result = serializer_type();
      result << SERVICE_ERROR << id << e;
    }
    catch (std::exception const &e)
    {
      fetch::logger.Error("Serialization error (Function Call3): ", e.what());
    }

    FETCH_LOG_DEBUG(LOGGING_NAME,"Service Server responding to call from ", client);
    {
      LOG_STACK_TRACE_POINT;
      DeliverResponse(client, result.data());
    }
    return ret;
  }
  bool HandleSubscribeRequest(connection_handle_type client, serializer_type params)
  {
    LOG_STACK_TRACE_POINT;
    bool ret = true;
    protocol_handler_type     protocol;
    feed_handler_type         feed;
    subscription_handler_type subid;

    lock_type lock(mutex_);
    try
    {
      protocol_states_.Set(SUBSCRIBED);
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("ServerInterface::HandleSubscribeRequest BAD_STATE_TRANSITION: ",
                          ex.what(),
                          " client=",
                          client,
                          "  ",
                          IdentifyProtocolSystem());
      return true;
    }

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
      FETCH_LOG_ERROR(LOGGING_NAME,"Serialization error (Subscribe): ", e.what());
      // result = serializer_type();
      // result << SERVICE_ERROR << id << e;
      throw e;  // TODO: propagate error other other size
    }
    // DeliverResponse(client, result.data());
    return ret;
  }

  bool HandleUnsubscribeRequest(connection_handle_type client, serializer_type params)
  {
    LOG_STACK_TRACE_POINT;
    bool ret = true;
    protocol_handler_type     protocol;
    feed_handler_type         feed;
    subscription_handler_type subid;

    lock_type lock(mutex_);
    try
    {
      protocol_states_.Set(SUBSCRIBED);
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("ServerInterface::HandleUnsubscribeRequest BAD_STATE_TRANSITION: ",
                          ex.what(),
                          " client=",
                          client,
                          "  ",
                          IdentifyProtocolSystem());
      return true;
    }

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
      FETCH_LOG_ERROR(LOGGING_NAME,"Serialization error (Unsubscribe): ", e.what());
      // result = serializer_type();
      // result << SERVICE_ERROR << id << e;
      throw e;  // TODO: propagate error other other size
    }
    // DeliverResponse(client, result.data());
    return ret;
  }


  virtual void ConnectionDropped(connection_handle_type connection_handle)
  {
    try
    {
      protocol_states_.Set(DEAD);
      RemoveAllProtocols(connection_handle);
    }
    catch(std::exception &ex)
    {
      fetch::logger.Error("ServerInterface::ExecuteCall BAD_STATE_TRANSITION: ", ex.what());
      throw;
    }
  }

  virtual void RemoveAllProtocols(connection_handle_type connection_handle)
  {
    lock_type lock(mutex_);
    for(int protocol_number = 0; protocol_number < 256; protocol_number++)
    {
      if (members_[protocol_number])
      {
        FETCH_LOG_WARN(LOGGING_NAME,"ConnectionDropped removing handler for protocol " , protocol_number, " from connection handle ", connection_handle);
        members_[protocol_number] -> ConnectionDropped(connection_handle);
        members_[protocol_number] = 0;
      }
    }

    auto msg = std::string("ServerInterface::ConnectionDropped ")
      + "   "
      + DescribeProtocolSystem();
    FETCH_LOG_WARN(LOGGING_NAME,msg);
  }

  virtual std::string IdentifyProtocolSystem()
  {
    std::string r;

    r += "Protocols: <";
    r += std::to_string((unsigned long)(&members_));
    r += ">";

    return r;
  }

  virtual std::string DescribeProtocolSystem()
  {
    std::string r = IdentifyProtocolSystem();

    r += " [ ";
    int count = 0;
    for(int i=0;i<256;i++)
    {
      if (members_[i] != nullptr)
      {
        if (count)
        {
          r+= ", ";
        }
        r += std::to_string(i);
        count++;
      }
    }
    if (!count)
    {
      r+= "none";
    }
    r += " ]";
    return r;
  }

private:
  void ExecuteCall(serializer_type &result, connection_handle_type const &connection_handle,
                   serializer_type params)
  {
    LOG_STACK_TRACE_POINT;

    protocol_handler_type protocol_number;
    function_handler_type function_number;
    params >> protocol_number >> function_number;

    try
    {
      protocol_states_.Set(MESSAGES);
    }
    catch(std::range_error &ex)
    {
      fetch::logger.Error("ServerInterface::ExecuteCall ", ex.what(), " client=", connection_handle, "  ", IdentifyProtocolSystem());
      throw;
    }

    auto identifier = std::to_string(protocol_number)
      + ":"
      + std::to_string(function_number)
      + "@"
      + std::to_string(connection_handle)
      ;

    FETCH_LOG_DEBUG(LOGGING_NAME,"ServerInterface::ExecuteCall " + identifier);

    auto protocol_pointer = members_[protocol_number];
    if (protocol_pointer == nullptr)
    {
      auto msg = std::string("ServerInterface::ExecuteCall: Could not find protocol ")
        + identifier
        + "   "
        + DescribeProtocolSystem();

      if (protocol_states_.Get() == MESSAGES)
      {
        FETCH_LOG_WARN(LOGGING_NAME, msg);
        return;
        //throw serializers::SerializableException(error::PROTOCOL_NOT_FOUND, msg);
      }
      else
      {
        return;
      }
    }

    protocol_pointer -> ApplyMiddleware(connection_handle, params.data());

    auto &function = (*protocol_pointer)[function_number];

    FETCH_LOG_DEBUG(LOGGING_NAME,std::string("ServerInterface::ExecuteCall: ")
                        + identifier
                        + " expecting following signature "
                        + function.signature()
                        );

    // If we need to add client id to function arguments
    try
    {
      if (function.meta_data() & Callable::CLIENT_ID_ARG)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME,"Adding connection_handle ID meta data to ", identifier);
        CallableArgumentList extra_args;
        extra_args.PushArgument(&connection_handle);
        function(result, extra_args, params);
      }
      else
      {
        function(result, params);
      }
    }
    catch (serializers::SerializableException const &e)
    {
      std::string new_explanation = e.explanation()
        + std::string(" (Function signature: ")
        + function.signature()
        + std::string(") (Identification: ")
        + identifier;
      serializers::SerializableException e2(e.error_code(), new_explanation);
      throw e2;
    }
    catch (std::exception &ex)
    {
      //FETCH_LOG_ERROR(LOGGING_NAME,"ServerInterface::ExecuteCall - ", ex.what(), " - ", identifier);
    }
  }

  mutex_type mutex_{__LINE__, __FILE__};
  Protocol *members_[256] = {nullptr};  // TODO: Not thread-safe
  friend class FeedSubscriptionManager;
};
}  // namespace service
}  // namespace fetch
