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
  using mutex_type          = std::mutex;
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

public:

  static constexpr char const *LOGGING_NAME = "ThreadPoolImpl";


  static std::shared_ptr<ThreadPoolImplementation> Create(std::size_t threads)
  {
    return std::make_shared<ThreadPoolImplementation>(threads);
  }

  ThreadPoolImplementation(std::size_t threads) : number_of_threads_(threads)
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

  virtual void Post(event_function_type item)
  {
    if (!shutdown_.load())
    {
      work_.Post(item);
      cv_.notify_one();
    }
  }

  virtual void SetIdleWork(event_function_type idle_work)
  {
    lock_type lock(mutex_);
    idle_work_.Post(idle_work);
  }

  virtual void ProcessLoop()
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
        lock_type lock(mutex_);
        cv_.wait_for(
            lock,
            std::chrono::milliseconds(100));  // so the future work will get serviced eventually.
        // go round again.
      }
    }
  }

  virtual thread_state_type Poll()
  {
    {
      LOG_STACK_TRACE_POINT;
      lock_type lock(mutex_);
      if (shutdown_.load())
      {
        return THREAD_SHOULD_QUIT;
      }
    }

    thread_state_type r = THREAD_IDLE;
    {
      auto workdone = work_.Visit([this](WorkStore::work_item_type work){ this->ExecuteWorkload(work); }, 1);
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

  template<class WORKER_TARGET>
  void Start(WORKER_TARGET *owner, void (WORKER_TARGET::*memberfunc)(void))
  {
    auto cb = [owner,memberfunc](){ (owner->*memberfunc)(); };
    Start(cb);
  }

  // TODO(EJF): Protected?
  virtual void Start(std::function<void (void)> function )
  {
    // TODO(EJF): Should monitor the number of threads that are being created
    FETCH_LOG_DEBUG(LOGGING_NAME,"Starting thread manager: ", number_of_threads_);

    if (threads_.size() == 0)
    {
      for (std::size_t i = 0; i < number_of_threads_; ++i)
      {
        threads_.push_back(new std::thread(function));
      }
    }
  }

  virtual void Start()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(thread_mutex_);
    if (threads_.size() == 0)
    {
      shared_ptr_type self = shared_from_this();
      auto cb = [self]() { self->ProcessLoop(); };
      Start(cb);
    }
  }

  virtual thread_state_type TryIdleWork()
  {
    thread_state_type r = THREAD_IDLE;
    LOG_STACK_TRACE_POINT;
    if (work_.Visit([this](IdleWorkStore::work_item_type work){ this->ExecuteWorkload(work); }) > 0)
    {
      r = THREAD_WORKED;
    }
    // if we didn't get the lock, one thread is already doing future work --
    // leave it be.
    return r;
  }

  virtual thread_state_type TryFutureWork()
  {
    thread_state_type            r = THREAD_IDLE;
    auto cb = [this](FutureWorkStore::work_item_type work){ this->Post(work); };

    if (future_work_.Visit(cb, 1) > 0)
    {
      r = THREAD_WORKED;                      // We did something.
    }

    // if we didn't get the lock, one thread is already
    // doing future work -- leave it be.
    return r;
  }

  void Stop()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(thread_mutex_);

    bool tryingToKillFromThreadWeOwn = false;
    shutdown_.store(true);
    for (auto &thread : threads_)
    {
      if (std::this_thread::get_id() == thread->get_id())
      {
        tryingToKillFromThreadWeOwn = true;
      }
    }

    if (tryingToKillFromThreadWeOwn)
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"Thread pools must not be killed by a thread they own.");
    }

    FETCH_LOG_INFO(LOGGING_NAME,"Removing work");
    {
      future_work_.clear();
      idle_work_.clear();
      work_.clear();
    }


    FETCH_LOG_INFO(LOGGING_NAME,"Kill threads");
    if (threads_.size() != 0)
      {

      {
        lock_type lock(mutex_);
        cv_.notify_all();
      }

      for (auto &thread : threads_)
      {
        if (std::this_thread::get_id() != thread->get_id())
        {
          thread->join();
          delete thread;
        }
        else
        {
          FETCH_LOG_ERROR(LOGGING_NAME,
              "Thread pools must not be killed by a thread they own"
              "so I'm not going to try joining myself.");
          thread->detach();
          delete thread;
        }
      }

      threads_.clear();
    }
  }

  template <typename F>
  void Post(F &&f, uint32_t milliseconds)
  {
    if (!shutdown_.load())
    {
      future_work_.Post(f, milliseconds);
      lock_type lock(mutex_);
      cv_.notify_one();
    }
  }

private:
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

  mutable fetch::mutex::Mutex     thread_mutex_{__LINE__, __FILE__};

  mutable std::condition_variable cv_;
  mutable mutex_type              mutex_;
  std::atomic<bool>               shutdown_{false};
};

}  // namespace details

using ThreadPool = typename std::shared_ptr<details::ThreadPoolImplementation>;

inline ThreadPool MakeThreadPool(std::size_t threads = 1)
{
  return details::ThreadPoolImplementation::Create(threads);
}

}  // namespace network
}  // namespace fetch
