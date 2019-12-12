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

using Clock        = PromiseTask::Clock;
using Timepoint    = PromiseTask::Timepoint;
using Duration     = PromiseTask::Duration;
using Promise      = service::Promise;
using PromiseState = service::PromiseState;

/**
 * Compute the desired timeout or deadline for the monitored promise
 *
 * @param promise The promise to be monitored
 * @return The desired deadline time
 */
Timepoint CalculateDeadline(Promise const &promise)
{
  return promise->deadline();
}

/**
 * Compute the desired timeout or deadline for the monitored promise
 *
 * @param promise The promise to be monitored
 * @param timeout The timeout to be imposed
 * @return The desired deadline time
 */
Timepoint CalculateDeadline(Promise const &promise, Duration const &timeout)
{
  return std::min(CalculateDeadline(promise), promise->created_at() + timeout);
}

}  // namespace

PromiseTask::PromiseTask(Promise const &promise, Callback callback)
  : PromiseTask(promise, CalculateDeadline(promise), std::move(callback))
{}

PromiseTask::PromiseTask(Promise const &promise, Duration const &timeout, Callback callback)
  : PromiseTask(promise, CalculateDeadline(promise, timeout), std::move(callback))
{}

PromiseTask::PromiseTask(Promise promise, Timepoint const &deadline, Callback callback)
  : promise_(std::move(promise))
  , deadline_{deadline}
  , callback_{std::move(callback)}
{}

bool PromiseTask::IsReadyToExecute() const
{
  bool ready{false};

  if (complete_ || !promise_)
  {
    // Case 1: The promise is complete or in an invalid state. In either case should not be run
  }
  else if (promise_->state() != PromiseState::WAITING)
  {
    // Case 2: The promise has already come to a conclusion of some kind
    ready = true;
  }
  else if (Clock::now() >= deadline_)
  {
    // Case 3: The promise has not already come to a conclusion but we have exceeded the deadline

    // signal that the promise has timed out
    promise_->Timeout();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Explicitly marking the promise as timed out");
    ready = true;
  }

  return ready;
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

char const *PromiseTask::GetId() const
{
  return ("PromiseTask#" + std::to_string(promise_->id())).c_str();
}

bool PromiseTask::IsComplete() const
{
  return complete_ || (!promise_);
}

}  // namespace muddle
}  // namespace fetch
