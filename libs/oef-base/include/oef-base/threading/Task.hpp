#pragma once

#include <atomic>
#include <chrono>
#include <memory>

#include "oef-base/threading/ExitState.hpp"

class Taskpool;

class Task : public std::enable_shared_from_this<Task>
{
  friend class Taskpool;

public:
  static constexpr char const *LOGGING_NAME = "Task";

  enum TaskState
  {
    NOT_PENDING,
    PENDING,
    SUSPENDED,
    DONE,
  };

  virtual bool      IsRunnable(void) const = 0;
  virtual ExitState RunThunk(void);
  virtual ExitState run(void) = 0;
  virtual void      cancel(void);

  bool IsCancelled(void) const
  {
    return cancelled;
  }

  Task();
  virtual ~Task();

  bool submit(std::shared_ptr<Taskpool> pool, const std::chrono::milliseconds &delay);
  bool submit(const std::chrono::milliseconds &delay);
  bool submit(std::shared_ptr<Taskpool> pool);
  bool submit();

  virtual bool MakeRunnable();

  virtual uint16_t GetMissedMakeRunnableCallsAndClear()
  {
    return missed_make_runnable_.exchange(0);
  }

  std::size_t GetTaskId()
  {
    return task_id;
  }
  std::size_t GetGroupId()
  {
    return group_id;
  }

  std::size_t SetGroupId(std::size_t new_group_id);
  static void SetThreadGroupId(std::size_t new_group_id);

  template <typename Derived>
  std::shared_ptr<Derived> shared_from_base()
  {
    return std::dynamic_pointer_cast<Derived>(shared_from_this());
  }

  void SetTaskState(TaskState state)
  {
    task_state_.store(state);
  }

  TaskState GetTaskState() const
  {
    return task_state_.load();
  }

private:
  std::shared_ptr<Taskpool> pool;
  std::atomic<TaskState>    task_state_;
  std::atomic<bool>         cancelled;
  std::size_t               group_id;
  std::size_t               task_id;
  std::atomic<uint16_t>     missed_make_runnable_;
};
