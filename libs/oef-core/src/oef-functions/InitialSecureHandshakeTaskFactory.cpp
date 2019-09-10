#include "InitialSecureHandshakeTaskFactory.hpp"

#include "mt-core/comms/src/cpp/OefAgentEndpoint.hpp"
#include "mt-core/oef-functions/src/cpp/OefFunctionsTaskFactory.hpp"
#include "mt-core/tasks-oef-base/TSendProtoTask.hpp"
#include "protos/src/protos/agent.pb.h"

void InitialSecureHandshakeTaskFactory::processMessage(ConstCharArrayBuffer &data)
{
  // TODO(LR)
}
