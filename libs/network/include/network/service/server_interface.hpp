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
  using Handle         = network::AbstractConnection::ConnectionHandleType;
  using ConstByteArray = byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "ServiceServerInterface";

  ServiceServerInterface() = default;

  virtual ~ServiceServerInterface() = default;

  void Add(ProtocolHandlerType const &name,
           Protocol *                 protocol)  // TODO(issue 19): Rename to AddProtocol
  {
    if (name < 1 || name > 255)
    {
      throw serialisers::SerializableException(
          error::PROTOCOL_RANGE,
          ConstByteArray(std::to_string(name) + " is out of protocol range."));
    }

    // TODO(issue 19): better reporting of errors
    if (members_[name] != nullptr)
    {
      throw serialisers::SerializableException(error::PROTOCOL_EXISTS, "Protocol already exists. ");
    }

    members_[name] = protocol;
  }

protected:
  virtual bool DeliverResponse(ConstByteArray const &address, network::MessageBuffer const &) = 0;

  bool PushProtocolRequest(ConstByteArray const &address, network::MessageBuffer const &msg,
                           CallContext const &context = CallContext())
  {
    SerialiserType            params(msg);
    ServiceClassificationType type;
    params >> type;

    FETCH_LOG_DEBUG(LOGGING_NAME, "PushProtocolRequest type=", type);

    bool success = false;

    switch (type)
    {
    case SERVICE_FUNCTION_CALL:
      success = HandleRPCCallRequest(address, params, context);
      break;
    default:
      FETCH_LOG_WARN(LOGGING_NAME, "PushProtocolRequest type not recognised ", type);
      break;
    }

    return success;
  }

  bool HandleRPCCallRequest(ConstByteArray const &address, SerialiserType params,
                            CallContext const &context = CallContext())
  {
    bool           ret = true;
    SerialiserType result;
    PromiseCounter id;

    try
    {
      params >> id;
      FETCH_LOG_DEBUG(LOGGING_NAME, "HandleRPCCallRequest prom =", id);
      result << SERVICE_RESULT << id;

      ExecuteCall(result, params, context);
    }
    catch (serialisers::SerializableException const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Serialization error (Function Call): ", e.what());
      result = SerialiserType();
      result << SERVICE_ERROR << id << e;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Service Server responding to call from ", address.ToHex(),
                    " data size=", result.tell());

    {
      DeliverResponse(address, result.data());
    }
    return ret;
  }

private:
  void ExecuteCall(SerialiserType &result, SerialiserType params,
                   CallContext const &context = CallContext())
  {
    ProtocolHandlerType protocol_number;
    FunctionHandlerType function_number;
    params >> protocol_number >> function_number;

    auto protocol_pointer = members_[protocol_number];
    if (protocol_pointer == nullptr)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "ServerInterface::ExecuteCall: Could not find protocol ",
                     protocol_number, ":", function_number);
      throw serialisers::SerializableException(error::PROTOCOL_NOT_FOUND,
                                               "Could not find protocol");
    }

    auto function = (*protocol_pointer)[function_number];

    FETCH_LOG_DEBUG(LOGGING_NAME, std::string("ServerInterface::ExecuteCall: "), protocol_number,
                    " expecting following signature " + function->signature());

    // If we need to add client id to function arguments
    try
    {
      CallableArgumentList extra_args;

      if ((function->meta_data() & Callable::CLIENT_CONTEXT_ARG) != 0u)
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Adding call context meta data to ", function_number);
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
    catch (serialisers::SerializableException const &e)
    {
      std::string new_explanation =
          e.explanation() + std::string(" (Function signature: ") + function->signature() + ")";

      FETCH_LOG_INFO(LOGGING_NAME, "EXCEPTION:", e.error_code(), new_explanation);

      throw serialisers::SerializableException(e.error_code(), new_explanation);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "ServerInterface::ExecuteCall - ", ex.what());
      throw serialisers::SerializableException(0, ex.what());
    }
  }

  Protocol *members_[256] = {nullptr};  // TODO(issue 19): Not thread-safe
};

}  // namespace service
}  // namespace fetch
