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
public:
  static constexpr char const *LOGGING_NAME = "ServiceClientInterface";

  // Construction / Destruction
  ServiceClientInterface();
  virtual ~ServiceClientInterface() = default;

  template <typename... arguments>

  Promise Call(uint32_t /*network_id*/, protocol_handler_type const &protocol,

               function_handler_type const &function, arguments &&... args)
  {
    LOG_STACK_TRACE_POINT;
    FETCH_LOG_DEBUG(LOGGING_NAME, "Service Client Calling ", protocol, ":", function);

    Promise prom = MakePromise(protocol, function);

    serializer_type params;

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

  Promise CallWithPackedArguments(protocol_handler_type const &protocol,
                                  function_handler_type const &function,
                                  byte_array::ByteArray const &args);

protected:
  virtual bool DeliverRequest(network::message_type const &request) = 0;

  bool ProcessServerMessage(network::message_type const &msg);
  void ProcessRPCResult(network::message_type const &msg, service::serializer_type &params);

private:
  void    AddPromise(Promise const &promise);
  Promise LookupPromise(PromiseCounter id);
  Promise ExtractPromise(PromiseCounter id);
  void    RemovePromise(PromiseCounter id);

  std::map<PromiseCounter, Promise> promises_;
  fetch::mutex::Mutex               promises_mutex_;
};
}  // namespace service
}  // namespace fetch
