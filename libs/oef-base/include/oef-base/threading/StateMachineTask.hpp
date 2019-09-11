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

#include "logging/logging.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Task.hpp"

template <class SUBCLASS>
class StateMachineTask : public Task
{
public:
  using Result     = std::pair<int, ExitState>;
  using EntryPoint = Result (SUBCLASS::*)(void);

  StateMachineTask(SUBCLASS *ptr, const EntryPoint *const entrypoints)
    : Task()
    , entrypoints(entrypoints)
  {
    this->ptr   = ptr;
    this->state = entrypoints[0];
    runnable    = true;
  }
  virtual ~StateMachineTask()
  {}

  EntryPoint const *           entrypoints;
  static constexpr char const *LOGGING_NAME = "StateMachineTask";

  virtual bool isRunnable(void) const
  {
    return runnable;
  }

  virtual ExitState run(void)
  {
    while (true)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Call state function");
      Result result = (ptr->*state)();

      if (result.first)
      {
        state = entrypoints[result.first];
      }
      else
      {
        state = 0;
      }
      FETCH_LOG_INFO(LOGGING_NAME, "Reply was ", result.first, ":", exitStateNames[result.second]);
      switch (result.second)
      {

      case COMPLETE:
        if (0 == state)
        {
          return COMPLETE;
        }
        break;

      case DEFER:
        return DEFER;

      case CANCELLED:
      case ERRORED:
        return result.second;
      }
    }
  }

protected:
  EntryPoint        state;
  std::atomic<bool> runnable;
  SUBCLASS *        ptr;

private:
  StateMachineTask(const StateMachineTask &other) = delete;  // { copy(other); }
  StateMachineTask &operator                      =(const StateMachineTask &other) =
      delete;                                               // { copy(other); return *this; }
  bool operator==(const StateMachineTask &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const StateMachineTask &other)  = delete;  // const { return compare(other)==-1; }
};
