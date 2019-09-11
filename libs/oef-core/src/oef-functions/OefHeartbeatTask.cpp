#include "oef-core/oef-functions/OefHeartbeatTask.hpp"

#include "agent.pb.h"
#include <iostream>
#include <string>

#include "oef-core/comms/OefAgentEndpoint.hpp"
#include "oef-core/tasks-base/TSendProtoTask.hpp"

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
