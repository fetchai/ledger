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

#include "promise_runnable.hpp"

namespace fetch {
namespace muddle {
namespace {

constexpr char const *LOGGING_NAME = "PromiseTask";

using Clock     = PromiseTask::Clock;
using Timepoint = PromiseTask::Timepoint;
using Duration  = PromiseTask::Duration;
using Promise   = service::Promise;
using PromiseState = service::PromiseState;

/**
 * Compute the desired timeout or deadline for the monitored promise
 *
 * @param promise The promise to be monitored
 * @param timeout The (optional) timeout to be imposed
 * @return The desired deadline time
 */
Timepoint CalculateDeadline(Promise const &promise, Duration const *timeout = nullptr)
{
  Timepoint deadline = promise->deadline();

  if (timeout)
  {
    deadline = std::min(deadline, promise->created_at() + *timeout);
  }

  return deadline;
}

}  // namespace

PromiseTask::PromiseTask(Promise const &promise, Callback callback)
  : PromiseTask(promise, CalculateDeadline(promise), std::move(callback))
{}

PromiseTask::PromiseTask(Promise const &promise, Duration const &timeout,
                         Callback callback)
  : PromiseTask(promise, CalculateDeadline(promise, &timeout), std::move(callback))
{}

PromiseTask::PromiseTask(Promise promise, Timepoint const &deadline, Callback callback)
  : promise_(std::move(promise))
  , deadline_{deadline}
  , callback_{std::move(callback)}
{}

bool PromiseTask::IsReadyToExecute() const
{
  bool ready{false};

  if (!complete_)
  {
    if (promise_)
    {
      // normal promise resolution
      ready = promise_->state() != PromiseState::WAITING;

      // if still not ready then check the deadline
      if (!ready)
      {
        if (Clock::now() >= deadline_)
        {
          // signal that the promise has timed out
          promise_->Timeout();

          FETCH_LOG_WARN(LOGGING_NAME, "Explicitly marking the transaction as timed out");
          ready = true;
        }
      }
    }
  }

  return ready;
  return (!complete_) && promise_ && (promise_->state() != service::PromiseState::WAITING);
}

void PromiseTask::Execute()
{
  try
  {
    if (callback_)
    {
      // execute the callback
      callback_(promise_);
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error generated while executing callback: ", ex.what());
  }

  complete_ = true;
}

bool PromiseTask::IsComplete() const
{
  return complete_ || (!promise_);
}

}  // namespace muddle
}  // namespace fetch
