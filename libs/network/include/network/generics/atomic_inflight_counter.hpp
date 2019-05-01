#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include <condition_variable>

#include "core/future_timepoint.hpp"
#include "core/logger.hpp"

namespace fetch {
namespace network {

enum class AtomicCounterName
{
  TCP_PORT_STARTUP,
  LOCAL_SERVICE_CONNECTIONS,
};

/**
 * This is a count of the number of instances of a type which have
 * been created but have not yet signalled that they are completely
 * set up. It includes a wait operation so code can make sure all its
 * dependencies are ready before proceeding.
 *
 * @tparam AtomicCounterName The name of the counter that this
 * instance will refer to from the above enumeration.
 */
template <AtomicCounterName>
class AtomicInFlightCounter
{
public:
  static constexpr char const *LOGGING_NAME = "AtomicInFlightCounter";

  AtomicInFlightCounter()
  {
    Counter &counter = GetCounter();

    Lock lock(counter.mutex);
    ++counter.total;
  }
  ~AtomicInFlightCounter() = default;

  void Completed()
  {
    auto &counter = GetCounter();

    Lock lock(counter.mutex);
    ++counter.complete;
    counter.cv.notify_all();
  }

  static bool Wait(const core::FutureTimepoint &until)
  {
    auto &the_counter = GetCounter();

    while (!until.IsDue())
    {
      Lock lock(the_counter.mutex);

      if (the_counter.complete >= the_counter.total)
      {
        return true;
      }

      the_counter.cv.wait_for(lock, until.DueIn());
    }

    return false;
  }

private:
  using CondVar = std::condition_variable;
  using Mutex   = std::mutex;
  using Lock    = std::unique_lock<Mutex>;

  struct Counter
  {
    Mutex   mutex;
    CondVar cv;

    uint32_t complete = 0;
    uint32_t total    = 0;
  };

  static Counter &GetCounter()
  {
    static Counter counter_instance;
    return counter_instance;
  }
};

}  // namespace network
}  // namespace fetch
