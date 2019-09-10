#include "KarmaRefreshTask.hpp"

#include <iostream>
#include <string>

#include "IKarmaPolicy.hpp"

ExitState KarmaRefreshTask::run(void)
{

  std::chrono::high_resolution_clock::time_point this_execute = std::chrono::high_resolution_clock::now();
  std::chrono::milliseconds d = std::chrono::duration_cast<std::chrono::milliseconds>(this_execute - last_execute);
  last_execute = std::chrono::high_resolution_clock::now();

  //FETCH_LOG_INFO(LOGGING_NAME, "LOGGED KarmaRefreshTask RUN ms=" + std::to_string(d.count()));
  policy -> refreshCycle(d);

  submit(std::chrono::milliseconds(interval));
  return COMPLETE;
}
