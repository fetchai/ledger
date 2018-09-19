#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <string>

#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"

#include "network/details/future_work_store.hpp"
#include "network/details/idle_work_store.hpp"
#include "network/details/work_store.hpp"
#include "network/generics/milli_timer.hpp"

namespace fetch {
namespace network {
namespace details {

class ThreadPoolImplementation : public std::enable_shared_from_this<ThreadPoolImplementation>
{
protected:
  using event_function_type = std::function<void()>;
  using event_handle_type   = uint64_t;
  using shared_ptr_type     = std::shared_ptr<ThreadPoolImplementation>;
  using mutex_type          = fetch::mutex::Mutex;
  using lock_type           = std::unique_lock<mutex_type>;

  using work_queue_type     = WorkStore;
  using future_work_type    = FutureWorkStore;
  using idle_work_type      = IdleWorkStore;

  enum thread_state_type
  {
    THREAD_IDLE        = 0x00,
    THREAD_WORKED      = 0x01,
    THREAD_SHOULD_QUIT = 0x02,
  };

  enum thread_activity
  {
    Starting = 0,
    Running = 1,
    Sleeping = 2,
    Working = 3,
    Joinable = 4,
  };

public:
  static constexpr char const *LOGGING_NAME = "ThreadPoolImpl";

  static std::shared_ptr<ThreadPoolImplementation> Create(std::size_t threads)
  {
    return std::make_shared<ThreadPoolImplementation>(threads);
  }

  ThreadPoolImplementation(std::size_t threads)
    : number_of_threads_(threads)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME,"Creating thread manager");
  }

  ThreadPoolImplementation(ThreadPoolImplementation const &) = delete;

  ThreadPoolImplementation(ThreadPoolImplementation &&) = default;

  virtual ~ThreadPoolImplementation()
  {
    Stop();
    FETCH_LOG_DEBUG(LOGGING_NAME,"Destroying thread manager");
  }

  virtual void SetIdleInterval()
  {
  }

  template <typename F>
  void Post(F &&f, uint32_t milliseconds)
  {
    if (!shutdown_.load())
    {
      std::unique_lock<mutex_type> lock(future_work_mutex_);
      StartOneThread();
      future_work_.Post(f, milliseconds);
      cv_.notify_one();
    }
  }

  virtual void Post(event_function_type item)
  {
    if (!shutdown_.load())
    {
      StartOneThread();
      work_.Post(item);
      cv_.notify_one();
    }
  }

  virtual void SetInterval(int milliseconds)
  {
    std::unique_lock<mutex_type> lock(idle_work_mutex_);
    idle_work_.SetInterval(milliseconds);
  }

  virtual void PostIdle(event_function_type idle_work)
  {
    if (!shutdown_.load())
    {
      StartOneThread();
      std::unique_lock<mutex_type> lock(idle_work_mutex_);
      idle_work_.Post(idle_work);
      cv_.notify_one();
    }
  }

  bool StartOneThread()
  {
    if (tc_.load() < number_of_threads_)
    {
      std::lock_guard<fetch::mutex::Mutex> lock(thread_mutex_);
      if (threads_.size() < number_of_threads_)
      {
        auto x = new std::thread([this](){
            this -> ProcessLoop();
          });
        threads_.push_back(x);
        tc_++;
      }
      return true;
    }
    return false;
  }

  virtual void clear(void)
  {
    future_work_.clear();
    idle_work_.clear();
    work_.clear();
  }

  virtual void Start()
  {
    // Done by magic now.
  }

  void Stop()
  {
    {
      LOG_STACK_TRACE_POINT;
      std::lock_guard<fetch::mutex::Mutex> lock(thread_mutex_);
      shutdown_.store(true);
      future_work_.Abort();
      idle_work_.Abort();
      work_.Abort();
    }
    cv_.notify_all();

    for (auto &thread : threads_)
    {
      if (std::this_thread::get_id() == thread->get_id())
      {
        FETCH_LOG_ERROR(LOGGING_NAME,"Thread pools must not be killed by a thread they own.");
        return;
      }
    }

    cv_.notify_all();

    {
      LOG_STACK_TRACE_POINT;
      std::lock_guard<fetch::mutex::Mutex> lock(thread_mutex_);
      FETCH_LOG_DEBUG(LOGGING_NAME,"Removing work");
      future_work_.clear();
      idle_work_.clear();
      work_.clear();
    }

    FETCH_LOG_DEBUG(LOGGING_NAME,"Removed work");
    cv_.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // TODO(EJF): Is this needed since the next operation if to wait for the threads to complete?
    // KLL -- I think so. I'm trying to get any remaining unstarted
    // threads to start, so they can read their terminate flag and
    // exit. If they're not started they're not joinable and will
    // error.
    FETCH_LOG_DEBUG(LOGGING_NAME,"Kill threads");
    for (auto &thread : threads_)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,
                     "Kill threads: Collect ",
                     thread->get_id()
                     );
      thread -> join();
    }

    FETCH_LOG_DEBUG(LOGGING_NAME,"Delete threads");
    for (auto &thread : threads_)
    {
      delete thread; // TODO(EJF): Should use smart pointers here
    }

    threads_.clear();
  }
private:

  virtual void ProcessLoop()
  {
    try
    {
      while (!shutdown_.load())
      {
        thread_state_type state = Poll();
        if (state & THREAD_SHOULD_QUIT)
        {
          return;  // we're done -- return & become joinable.
        }
        if (state & THREAD_WORKED)
        {
          // no delay, go do more.
          continue;
        }
        // THREAD_IDLE:
        {
          // snooze for a while or until more work arrives
          LOG_STACK_TRACE_POINT;

          std::unique_lock<std::mutex> lock(mutex_);

          auto future_work_wait_duration = future_work_ . DueIn();
          auto idle_work_wait_duration = idle_work_ . DueIn();

          auto minimum_wait_duration = std::chrono::milliseconds(0);

          //FETCH_LOG_WARN(LOGGING_NAME,"future_work_ due in milliseconds:", std::chrono::milliseconds(future_work_wait_duration).count());
          //FETCH_LOG_WARN(LOGGING_NAME,"idle_work_ due in milliseconds:", std::chrono::milliseconds(idle_work_wait_duration).count());
          //FETCH_LOG_WARN(LOGGING_NAME,"minimum_ due in milliseconds:", std::chrono::milliseconds(minimum_wait_duration).count());

          auto wait_duration = std::max(std::min(future_work_wait_duration, idle_work_wait_duration), minimum_wait_duration);

          //FETCH_LOG_WARN(LOGGING_NAME,"Sleeping for milliseconds:", std::chrono::milliseconds(wait_duration).count());
          cv_.wait_for(lock, wait_duration);
          // go round again.
        }
      }
    }
    catch (...)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"OMG, bad lock in thread_pool");
      throw;
    }
  }

  virtual thread_state_type Poll()
  {
    {
      LOG_STACK_TRACE_POINT;
      if (shutdown_.load())
      {
        return THREAD_SHOULD_QUIT;
      }
    }

    thread_state_type r = THREAD_IDLE;
    {
      auto workdone = work_.Visit([this](WorkStore::WorkItem work){ this->ExecuteWorkload(work); }, 1);
      if (workdone > 0)
      {
        r = std::max(r, THREAD_WORKED);
      }
    }

    if (shutdown_.load())
    {
      return THREAD_SHOULD_QUIT;
    }

    if (!r)
    {
      r = std::max(r, TryFutureWork());
    }

    if (shutdown_)
    {
      return THREAD_SHOULD_QUIT;
    }

    if (!r)
    {
      r = std::max(r, TryIdleWork());
    }

    if (shutdown_.load())
    {
      return THREAD_SHOULD_QUIT;
    }

    return r;
  }

  virtual thread_state_type TryIdleWork()
  {
    thread_state_type r = THREAD_IDLE;
    LOG_STACK_TRACE_POINT;

    std::unique_lock<mutex_type> lock(idle_work_mutex_, std::try_to_lock);
    if (!lock)
    {
      return r;
    }

    if (idle_work_.IsDue())
    {
      if (idle_work_.Visit([this](IdleWorkStore::work_item_type work){ this->ExecuteWorkload(work); }) > 0)
      {
        r = THREAD_WORKED;
      }
    }
    // if we didn't get the lock, one thread is already doing future work --
    // leave it be.
    return r;
  }

  virtual thread_state_type TryFutureWork()
  {
    thread_state_type            r = THREAD_IDLE;
    auto cb = [this](FutureWorkStore::work_item_type work){ this->Post(work); };

    std::unique_lock<mutex_type> lock(future_work_mutex_, std::try_to_lock);
    if (!lock)
    {
      return r;
    }

    if (future_work_.IsDue())
    {
      if (future_work_.Visit(cb, 1) > 0)
      {
        r = THREAD_WORKED;                      // We did something.
      }
    }

    // if we didn't get the lock, one thread is already
    // doing future work -- leave it be.
    return r;
  }

  virtual thread_state_type ExecuteWorkload(event_function_type workload)
  {
    if (shutdown_.load())
    {
      return THREAD_SHOULD_QUIT;
    }

    thread_state_type r = THREAD_IDLE;
    try
    {
      LOG_STACK_TRACE_POINT;
      workload();
      r = std::max(r, THREAD_WORKED);
    }
    catch (std::exception &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"Caught exception in ThreadPool::ExecuteWorkload - ", ex.what());
    }
    catch (...)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"Caught exception in ThreadPool::ExecuteWorkload");
    }

    if (shutdown_.load())
    {
      return THREAD_SHOULD_QUIT;
    }

    return r;
  }

  std::size_t                number_of_threads_ = 1;
  std::vector<std::thread *> threads_;

  future_work_type future_work_;
  work_queue_type  work_;
  idle_work_type   idle_work_;

  mutable mutex_type  idle_work_mutex_{__LINE__, __FILE__};
  mutable mutex_type  future_work_mutex_{__LINE__, __FILE__};

  mutable fetch::mutex::Mutex     thread_mutex_{__LINE__, __FILE__};

  mutable std::condition_variable cv_;
  mutable mutex_type              mutex_{__LINE__, __FILE__};
  std::atomic<bool>               shutdown_{false};
  std::atomic<unsigned long>                tc_{0};

  
};

}  // namespace details

using ThreadPool = typename std::shared_ptr<details::ThreadPoolImplementation>;

inline ThreadPool MakeThreadPool(std::size_t threads = 1)
{
  return details::ThreadPoolImplementation::Create(threads);
}

}  // namespace network
}  // namespace fetch
