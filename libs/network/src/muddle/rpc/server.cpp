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
