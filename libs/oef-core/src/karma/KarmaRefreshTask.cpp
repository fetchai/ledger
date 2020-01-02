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

#include "oef-core/karma/KarmaRefreshTask.hpp"

#include <iostream>
#include <string>

#include "oef-core/karma/IKarmaPolicy.hpp"

fetch::oef::base::ExitState KarmaRefreshTask::run()
{
  std::chrono::high_resolution_clock::time_point this_execute =
      std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds d =
      std::chrono::duration_cast<std::chrono::milliseconds>(this_execute - last_execute);
  last_execute = std::chrono::high_resolution_clock::now();

  // FETCH_LOG_INFO(LOGGING_NAME, "LOGGED KarmaRefreshTask RUN ms=" + std::to_string(d.count()));
  policy->RefreshCycle(d);

  submit(std::chrono::milliseconds(interval));
  return fetch::oef::base::COMPLETE;
}
