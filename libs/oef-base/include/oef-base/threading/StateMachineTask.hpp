#pragma once

#include "core/logging.hpp"
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
