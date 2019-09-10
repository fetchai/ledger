#include "OefHeartbeatTask.hpp"

#include <iostream>
#include <string>
#include "protos/src/protos/agent.pb.h"

#include "mt-core/comms/src/cpp/OefAgentEndpoint.hpp"
#include "mt-core/tasks-base/src/cpp/TSendProtoTask.hpp"

ExitState OefHeartbeatTask::run(void)
{
  auto sp = ep . lock();
  if (sp)
  {
    sp -> heartbeat();
    submit(std::chrono::milliseconds(1000));
    return COMPLETE;
  }
  else
  {
    return COMPLETE;
  }
}
