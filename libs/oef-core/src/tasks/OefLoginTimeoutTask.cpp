//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "oef-core/tasks/OefLoginTimeoutTask.hpp"

#include <iostream>
#include <string>

#include "oef-core/comms/OefAgentEndpoint.hpp"

fetch::oef::base::ExitState OefLoginTimeoutTask::run()
{
  auto sp = ep.lock();
  if (sp)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "id=", sp->GetIdentifier(), " TIMEOUT");
    if (!sp->GetState("loggedin"))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "TIMEOUT");
      sp->close("login-timeout");
    }
  }
  return fetch::oef::base::COMPLETE;
}
