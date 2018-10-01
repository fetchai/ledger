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

#include "network/generics/future_timepoint.hpp"

namespace fetch {
namespace network {

/**
 * Simple wrapper around the service promise which mandates the return time from the underlying
 * promise.
 *
 * @tparam RESULT The expected return type of the promise
 */
template <typename DIFFERENTIATOR>
class AtomicInflightCounter
{
  using Counter = std::atomic<unsigned int>;
  using CondVar = std::condition_variable;
  using Mutex   = std::mutex;
  using Lock    = std::unique_lock<Mutex>;

private:
  using TheCounter = struct
  {
    Counter count;
    CondVar cv;
    Mutex   mutex;
  };

  static TheCounter &GetCounter()
  {
    static TheCounter theCounter;
    return theCounter;
  }

  unsigned int my_count_;

public:
  static constexpr char const *LOGGING_NAME = "AtomicInflightCounter<>";

  AtomicInflightCounter(unsigned int my_count = 1)
  {
    my_count_     = my_count;
    auto previous = GetCounter().count.fetch_add(my_count_);
  }

  void Completed(unsigned int completed_count)
  {
    unsigned int clipped = std::min(completed_count, my_count_);
    my_count_ -= clipped;
    auto previous = GetCounter().count.fetch_sub(clipped);
    if (previous == 1)
    {
      GetCounter().cv.notify_all();
    }
  }

  ~AtomicInflightCounter()
  {
    Completed(my_count_);
  }

  static bool Wait(const FutureTimepoint &until)
  {
    auto &the_counter = GetCounter();
    while (1)
    {
      if (until.IsDue())
      {
        return false;
      }
      if (the_counter.count.load() == 0)
      {
        return true;
      }
      {
        Lock lock(the_counter.mutex);
        the_counter.cv.wait_for(lock, until.DueIn());
      }
    }
  }
};

}  // namespace network
}  // namespace fetch
