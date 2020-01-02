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

#include "core/mutex.hpp"
#include "core/set_thread_name.hpp"

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
  Pool()
    : Pool(2 * std::thread::hardware_concurrency())
  {}

  explicit Pool(std::size_t n, std::string name = std::string{})
    : concurrency_{n}
    , name_{std::move(name)}
  {
    for (std::size_t i = 0; i < n; ++i)
    {
      workers_.emplace_back([this, i]() {
        if (!name_.empty())
        {
          SetThreadName(name_, i);
        }

        this->Work();
      });
    }
  }

  ~Pool()
  {
    running_ = false;

    {
      FETCH_LOCK(mutex_);
      condition_.notify_all();
    }

    for (auto &w : workers_)
    {
      w.join();
    }
  }

  template <typename F, typename... Args>
  std::future<typename std::result_of<F(Args...)>::type> Dispatch(F &&f, Args &&... args)
  {
    using ReturnType = typename std::result_of<F(Args...)>::type;
    using TaskType   = std::packaged_task<ReturnType()>;

    std::shared_ptr<TaskType> task =
        std::make_shared<TaskType>(std::bind(f, std::forward<Args>(args)...));

    {
      FETCH_LOCK(mutex_);
      tasks_.emplace([task]() { (*task)(); });
    }

    condition_.notify_one();
    return task->get_future();
  }

  void Wait()
  {
    while (!Empty())
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
    while (tasks_in_progress_ != 0)
    {
      std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
  }

  bool Empty()
  {
    FETCH_LOCK(mutex_);
    return tasks_.empty();
  }

  std::size_t concurrency() const
  {
    return concurrency_;
  }

private:
  void Work()
  {
    while (running_)
    {
      auto task = NextTask();
      if (task)
      {
        task();
        --tasks_in_progress_;
      }
    }
  }

  std::function<void()> NextTask()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return (!bool(running_)) || (!tasks_.empty()); });
    if (!bool(running_))
    {
      return {};
    }

    auto task = tasks_.front();
    ++tasks_in_progress_;

    tasks_.pop();
    return task;
  }

  std::size_t const                 concurrency_;
  std::string                       name_{};
  std::atomic<uint32_t>             tasks_in_progress_{0};
  std::atomic<bool>                 running_{true};
  std::mutex                        mutex_;
  std::vector<std::thread>          workers_;
  std::queue<std::function<void()>> tasks_;
  std::condition_variable           condition_;
};

}  // namespace threading
}  // namespace fetch
