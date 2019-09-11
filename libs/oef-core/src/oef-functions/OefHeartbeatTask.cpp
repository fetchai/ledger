#include "oef-core/oef-functions/OefHeartbeatTask.hpp"

#include "agent.pb.h"
#include <iostream>
#include <string>

#include "mt-core/tasks-oef-base/TSendProtoTask.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"

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
