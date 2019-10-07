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

  virtual bool      isRunnable(void) const = 0;
  virtual ExitState runThunk(void);
  virtual ExitState run(void) = 0;
  virtual void      cancel(void);

  bool isCancelled(void) const
  {
    return cancelled;
  }

  Task();
  virtual ~Task();

  bool submit(std::shared_ptr<Taskpool> pool, const std::chrono::milliseconds &delay);
  bool submit(const std::chrono::milliseconds &delay);
  bool submit(std::shared_ptr<Taskpool> pool);
  bool submit();

  virtual void makeRunnable();

  std::size_t getTaskId()
  {
    return task_id;
  }
  std::size_t getGroupId()
  {
    return group_id;
  }

  std::size_t setGroupId(std::size_t new_group_id);
  static void setThreadGroupId(std::size_t new_group_id);

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
};
