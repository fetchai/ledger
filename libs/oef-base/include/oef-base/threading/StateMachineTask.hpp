#pragma once

#include "ExitState.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Task.hpp"

template <class SUBCLASS>
class StateMachineTask : virtual public Task
{
public:
  using Result     = std::pair<int, ExitState>;
  using EntryPoint = Result (SUBCLASS::*)(void);

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

  virtual ~StateMachineTask()
  {}

  EntryPoint const *           entrypoints;
  static constexpr char const *LOGGING_NAME = "StateMachineTask";

  virtual bool IsRunnable(void) const
  {
    return runnable;
  }

  void SetSubClass(SUBCLASS *ptr)
  {
    this->ptr = ptr;
    runnable  = true;
  }

  virtual ExitState run(void)
  {
    try
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
    catch (std::exception &e)
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
