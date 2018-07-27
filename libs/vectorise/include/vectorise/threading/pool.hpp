#pragma once

#include "core/mutex.hpp"

#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>

namespace fetch {
namespace threading {

class Pool
{
public:
  Pool() : Pool(2 * std::thread::hardware_concurrency()) {}

  Pool(std::size_t const &n)
  {
    running_ = true;
    for (std::size_t i = 0; i < n; ++i)
    {
      workers_.emplace_back([this]() { this->Work(); });
    }
  }

  ~Pool()
  {
    running_ = false;
    condition_.notify_all();
    for (auto &w : workers_) w.join();
  }

  template <typename F, typename... Args>
  std::future<typename std::result_of<F(Args...)>::type> Dispatch(F &&f, Args &&... args)
  {
    using return_type = typename std::result_of<F(Args...)>::type;
    using task_type = std::packaged_task<return_type()>;

    std::shared_ptr<task_type> task =
        std::make_shared<task_type>(std::bind(f, std::forward<Args>(args)...));

    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return task->get_future();
  }

  void Wait()
  {
    while (!Empty())
    {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }

  bool Empty()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    return tasks_.empty();
  }

private:
  void Work()
  {
    while (running_)
    {
      std::function<void()> task = NextTask();
      if (task) task();
    }
  }

  std::function<void()> NextTask()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return (!bool(running_)) || (!tasks_.empty()); });
    if (!bool(running_)) return std::function<void()>();

    std::function<void()> task = std::move(tasks_.front());
    tasks_.pop();
    return task;
  }

  std::atomic<bool>                 running_;
  std::mutex                        mutex_;
  std::vector<std::thread>          workers_;
  std::queue<std::function<void()>> tasks_;
  std::condition_variable           condition_;
};

};  // namespace threading
};  // namespace fetch
