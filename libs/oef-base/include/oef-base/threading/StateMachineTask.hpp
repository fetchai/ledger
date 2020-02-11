#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ExitState.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Task.hpp"

namespace fetch {
namespace oef {
namespace base {

template <class SUBCLASS>
class StateMachineTask : virtual public Task
{
public:
  using Result     = std::pair<int, ExitState>;
  using EntryPoint = Result (SUBCLASS::*)();

  StateMachineTask(SUBCLASS *ptr, const EntryPoint *const entrypoints)
    : Task()
    , entrypoints(entrypoints)
    , runnable{true}
  {
    this->ptr = ptr;
    if (entrypoints != nullptr)
    {
      this->state = entrypoints[0];
    }
  }
  StateMachineTask()
    : entrypoints{nullptr}
    , runnable{false}
    , ptr{nullptr}
  {}

  ~StateMachineTask() override = default;

  EntryPoint const *           entrypoints;
  static constexpr char const *LOGGING_NAME = "StateMachineTask";

  bool IsRunnable() const override
  {
    return runnable;
  }

  void SetSubClass(SUBCLASS *p)
  {
    this->ptr = p;
    runnable  = true;
  }

  ExitState run() override
  {
    try
    {

      while (true)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Call state function");
        Result result = (ptr->*state)();

        if (result.first != 0)
        {
          state = entrypoints[result.first];
        }
        else
        {
          state = 0;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "Reply was ", result.first, ":",
                       exitStateNames[result.second]);

        switch (result.second)
        {

        case COMPLETE:
          if (0 == state)
          {
            return COMPLETE;
          }
          break;
        case RERUN:
          return RERUN;

        case DEFER:
          return DEFER;

        case CANCELLED:
        case ERRORED:
          return result.second;
        }
      }
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Exception in calling state function: ", e.what());
      return ERRORED;
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

}  // namespace base
}  // namespace oef
}  // namespace fetch
