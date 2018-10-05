#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/logger.hpp"
#include "network/generics/future_timepoint.hpp"

namespace fetch {
namespace network {

enum class AtomicCounterName
{
  TCP_PORT_STARTUP,
};

/**
 * This is a count of the number of instances of a type which have
 * been created but have not yet signalled that they are completely
 * set up. It includes a wait operation so code can make sure all its
 * dependendies are ready before proceeding.
 *
 * @tparam AtomicCounterName The name of the counter that this
 * instance will refer to from the above enumeration.
 */
template <AtomicCounterName>
class AtomicInflightCounter
{
public:
private:
  using Counter = std::atomic<unsigned int>;
  using CondVar = std::condition_variable;
  using Mutex   = std::mutex;
  using Lock    = std::unique_lock<Mutex>;

  struct TheCounter
  {
    Mutex   mutex;
    CondVar cv;

    uint32_t complete = 0;
    uint32_t total = 0;
  };

  static TheCounter &GetCounter()
  {
    static TheCounter theCounter;
    return theCounter;
  }

public:
  static constexpr char const *LOGGING_NAME = "AtomicInflightCounter";

  AtomicInflightCounter()
  {
    auto &the_counter = GetCounter();

    Lock  lock(the_counter.mutex);
    ++the_counter.total;
  }

  ~AtomicInflightCounter()  = default;

  void Completed()
  {
    auto &the_counter = GetCounter();

    {
      Lock lock(the_counter.mutex);
      ++the_counter.complete;
    }

    the_counter.cv.notify_all();
  }

  static bool Wait(const FutureTimepoint &until)
  {
    auto &the_counter = GetCounter();

    while (!until.IsDue())
    {
      Lock lock(the_counter.mutex);

      if (the_counter.complete >= the_counter.total)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Waited: great success");
        return true;
      }

      the_counter.cv.wait_for(lock, until.DueIn());
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Waited: epic fail");

    return false;
  }
};

}  // namespace network
}  // namespace fetch
