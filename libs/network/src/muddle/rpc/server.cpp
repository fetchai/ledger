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

#include "network/muddle/rpc/server.hpp"

namespace fetch {
namespace muddle {
namespace rpc {

#if 0

using Serializer = service::serializer_type;
using ServiceClassification = service::service_classification_type;
using PromiseCounter = service::PromiseCounter;

void Server::PushRequest(ConnectionHandle client, network::message_type const &msg)
{
  LOG_STACK_TRACE_POINT;
  Serializer params(msg);

  ServiceClassification type;
  params >> type;

  Serializer     result;
  PromiseCounter id;

  try
  {
    LOG_STACK_TRACE_POINT;
    params >> id;
    result << service::SERVICE_RESULT << id;

    service::ServiceServerInterface::ExecuteCall(result, client, params);
  }
  catch (serializers::SerializableException const &e)
  {
    LOG_STACK_TRACE_POINT;
    FETCH_LOG_ERROR(LOGGING_NAME,"Serialization error (Function Call): ", e.what());

    result = Serializer();
    result << service::SERVICE_ERROR << id << e;
  }

  FETCH_LOG_DEBUG(LOGGING_NAME,"Service Server responding to call from ", client);
  {
    LOG_STACK_TRACE_POINT;
    DeliverResponse(client, result.data());
  }

}
#endif

} // namespace rpc
} // namespace muddle
} // namespace fetch
