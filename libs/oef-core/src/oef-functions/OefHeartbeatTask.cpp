#include "OefHeartbeatTask.hpp"

#include "protos/src/protos/agent.pb.h"
#include <iostream>
#include <string>

#include "mt-core/comms/src/cpp/OefAgentEndpoint.hpp"
#include "mt-core/tasks-oef-base/TSendProtoTask.hpp"

ExitState OefHeartbeatTask::run(void)
{
  auto sp = ep.lock();
  if (sp)
  {
    sp->heartbeat();
    submit(std::chrono::milliseconds(1000));
    return COMPLETE;
  }
  else
  {
    return COMPLETE;
  }
}
