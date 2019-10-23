#include "oef-core/oef-functions/OefHeartbeatTask.hpp"

#include "oef-messages/agent.hpp"
#include <iostream>
#include <string>

#include "oef-base/proto_comms/TSendProtoTask.hpp"
#include "oef-core/comms/OefAgentEndpoint.hpp"

ExitState OefHeartbeatTask::run(void)
{
  if (IsCancelled())
  {
    return CANCELLED;
  }
  auto sp = ep.lock();
  if (sp)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "running on connection id=", sp->GetIdentifier(),
                    "  task=", GetTaskId(), "  group=", GetGroupId());
    sp->heartbeat();
    submit(std::chrono::milliseconds(200));
    return COMPLETE;
  }
  else
  {
    return COMPLETE;
  }
}
