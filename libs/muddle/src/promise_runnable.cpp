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

static constexpr char const *LOGGING_NAME = "PromiseTask";

PromiseTask::PromiseTask(service::Promise promise, Callback callback)
  : promise_{std::move(promise)}
  , callback_{std::move(callback)}
{
}

bool PromiseTask::IsReadyToExecute() const
{
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


} // namespace muddle
} // namespace fetch
