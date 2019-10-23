#include "oef-core/tasks/OefLoginTimeoutTask.hpp"

#include <iostream>
#include <string>

#include "oef-core/comms/OefAgentEndpoint.hpp"

ExitState OefLoginTimeoutTask::run(void)
{
  auto sp = ep.lock();
  if (sp)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "???? id=", sp->GetIdentifier(), " TIMEOUT");
    if (!sp->GetState("loggedin"))
    {
      FETCH_LOG_INFO(LOGGING_NAME,
                     "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TIMEOUT");
      sp->close("login-timeout");
    }
  }
  return COMPLETE;
}
