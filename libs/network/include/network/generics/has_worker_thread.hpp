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

#include "core/threading.hpp"
#include "network/generics/resolvable.hpp"
#include "network/service/promise.hpp"

namespace fetch {
namespace network {

/**
 * This is a simple class to give someone a background worker
 * thread. Create it, give it a work function in a
 * suitable concept.
 *
 * The concept also needs to implement a Wait(int milliseconds) method
 * that will only return if there is work to be done or the timeout
 * has elapsed. (eg; uses a condition variable) and a WakeAll() method
 * which will trigger return from the Wait() method by all sleeping
 * threads (eg a cv's notify_all).
 *
 * @tparam TARGET The class type which will implement Wait/WakeAll for us.
 */
template <typename TARGET>
class HasWorkerThread
{
private:
  using Target   = TARGET;
  using WorkFunc = std::function<void()>;

public:
  static constexpr char const *LOGGING_NAME = "HasWorkerThread";

  HasWorkerThread(Target *target, std::string name, std::function<void()> workcycle)
    : target_(target)
    , workcycle_(std::move(workcycle))
    , name_{std::move(name)}
  {
    thread_ = std::make_shared<std::thread>([this]() { this->Run(); });
  }

  virtual ~HasWorkerThread()
  {
    shutdown_ = true;

    target_->WakeAll();
    target_->WakeAll();
    target_->WakeAll();

    if (thread_)
    {
      thread_->join();
    }
    thread_.reset();
  }

  void ChangeWaitTime(std::chrono::milliseconds wait_time)
  {
    wait_time_ = wait_time;
  }

protected:
  void Run()
  {
    SetThreadName(name_);

    if (!target_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No target configured, stopping thread.");
      return;
    }
    while (!shutdown_)
    {
      target_->Wait(wait_time_);
      if (shutdown_)
      {
        return;
      }
      workcycle_();
    }
  }

  using ShutdownFlag = std::atomic<bool>;
  using Thread       = std::thread;
  using ThreadPtr    = std::shared_ptr<Thread>;

  Target *                  target_{nullptr};
  ThreadPtr                 thread_;
  ShutdownFlag              shutdown_{false};
  WorkFunc                  workcycle_;
  std::string               name_;
  std::chrono::milliseconds wait_time_{100};
};

}  // namespace network
}  // namespace fetch
