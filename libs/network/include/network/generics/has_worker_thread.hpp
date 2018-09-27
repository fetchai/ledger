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

#include "network/generics/resolvable.hpp"
#include "network/service/promise.hpp"

namespace fetch {
namespace network {

/**
 * Simple wrapper around the service promise which mandates the return time from the underlying
 * promise.
 *
 * @tparam RESULT The expected return type of the promise
 */
template <typename TARGET>
class HasWorkerThread
{
private:
  using ShutdownFlag = std::atomic<bool>;
  using Thread       = std::thread;
  using ThreadP      = std::shared_ptr<Thread>;
  using Target       = TARGET;
  using WorkFunc     = std::function<void (void)>;
public:
  static constexpr char const *LOGGING_NAME = "HasWorkerThread";

  HasWorkerThread(Target *target, std::function<void (void)> workcycle)
  {
    workcycle_ = workcycle;
    target_ = target;
    if (!target_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No target configured in construction.");
    }
    thread_ = std::make_shared<std::thread>([this]() { this->Run(); });
  }

  virtual ~HasWorkerThread()
  {
    shutdown_.store(true);

    target_->WakeAll();
    target_->WakeAll();
    target_->WakeAll();

    if (thread_)
    {
      thread_->join();
    }
    thread_.reset();
  }

protected:
  void Run(void)
  {
    if (!target_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "No target configured, stopping thread.");
      return;
    }
    while(1)
    {
      if (shutdown_.load()) return;
      {
        target_->Wait(100);
      }
      if (shutdown_.load()) return;
      workcycle_();
      if (shutdown_.load()) return;
    }
  }
  Target *target_;
  ThreadP thread_;
  ShutdownFlag shutdown_;
  WorkFunc workcycle_;
};

}  // namespace network
}  // namespace fetch
