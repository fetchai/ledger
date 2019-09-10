#include "OefLoginTimeoutTask.hpp"

#include <iostream>
#include <string>

#include "mt-core/comms/src/cpp/OefAgentEndpoint.hpp"

ExitState OefLoginTimeoutTask::run(void)
{
  auto sp = ep . lock();
  if (sp)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "???? id=", sp -> getIdent() ," TIMEOUT");
    if (!sp -> getState("loggedin"))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! TIMEOUT");
      sp -> close("login-timeout");
    }
  }
  return COMPLETE;
}
