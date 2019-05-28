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

#include "core/logger.hpp"
#include "core/macros.hpp"

#include <atomic>
#include <csignal>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace fetch {
namespace mutex {

/**
 * The production mutex is wrapper for the default system mutex.
 */
class ProductionMutex : public AbstractMutex
{
public:
  ProductionMutex(int, std::string const &)
  {}
};

/**
 * The debug mutex acts like a normal mutex but also contains several other checks. This code is
 * intended to be only used in software development.
 */
class DebugMutex : public AbstractMutex
{
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;
  using Duration  = Clock::duration;

  static constexpr char const *LOGGING_NAME = "DebugMutex";

  struct LockInfo
  {
    bool locked = true;
  };

  class MutexTimeout
  {
  public:
    static constexpr std::size_t DEFAULT_TIMEOUT_MS = 3000;

    MutexTimeout(std::string filename, int const &line, std::size_t timeout_ms = DEFAULT_TIMEOUT_MS)
      : filename_(std::move(filename))
      , line_(line)
    {
      LOG_STACK_TRACE_POINT;

      thread_ = std::thread([=]() {
        LOG_LAMBDA_STACK_TRACE_POINT;

        // define the point at which the deadline has been reached
        Timepoint const deadline = created_ + std::chrono::milliseconds(timeout_ms);

        while (running_)
        {
          // exit waiting loop when the dead line has been reached
          if (Clock::now() >= deadline)
          {
            break;
          }

          // wait
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (running_)
        {
          this->Eval();
        }
      });
    }

    ~MutexTimeout()
    {
      running_ = false;
      thread_.join();
    }

    void Eval()
    {
      LOG_STACK_TRACE_POINT;
      FETCH_LOG_ERROR(LOGGING_NAME, "The system will terminate, mutex timed out: ", filename_, " ",
                      line_);

      // Send a sigint to ourselves since we have a handler for this
      kill(0, SIGINT);
    }

  private:
    std::string       filename_;
    int               line_;
    std::thread       thread_;
    Timepoint         created_ = Clock::now();
    std::atomic<bool> running_{true};
  };

public:
  DebugMutex(int line, std::string file)
    : AbstractMutex()
    , line_(line)
    , file_(std::move(file))
  {}

  // TODO(ejf) No longer required?
  DebugMutex() = delete;

  DebugMutex &operator=(DebugMutex const &other) = delete;

  void lock()
  {
    LOG_STACK_TRACE_POINT;
    lock_mutex_.lock();
    locked_ = Clock::now();
    lock_mutex_.unlock();

    std::mutex::lock();

    timeout_ = std::make_unique<MutexTimeout>(file_, line_);
    fetch::logger.RegisterLock(this);
    thread_id_ = std::this_thread::get_id();
  }

  void unlock()
  {
    LOG_STACK_TRACE_POINT;

    lock_mutex_.lock();
    Timepoint const end_time   = Clock::now();
    Duration const  delta_time = end_time - locked_;
    double          total_time = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(delta_time).count());

    lock_mutex_.unlock();

    timeout_.reset(nullptr);
    fetch::logger.RegisterUnlock(this, total_time, file_, line_);

    std::mutex::unlock();
  }

  int line() const
  {
    return line_;
  }

  std::string filename() const
  {
    return file_;
  }

  std::string AsString() override
  {
    std::stringstream ss;
    ss << "Locked by thread #" << fetch::log::ReadableThread::GetThreadID(thread_id_) << " in "
       << filename() << " on " << line();
    return ss.str();
  }

  std::thread::id thread_id() const override
  {
    return thread_id_;
  }

private:
  std::mutex lock_mutex_;
  Timepoint locked_ FETCH_GUARDED_BY(lock_mutex_);  ///< The time when the mutex was locked
  std::thread::id   thread_id_;                     ///< The last thread to lock the mutex

  int                           line_ = 0;   ///< The line number of the mutex
  std::string                   file_ = "";  ///< The filename of the mutex
  std::unique_ptr<MutexTimeout> timeout_;    ///< The timeout monitor for this mutex
};

#ifdef NDEBUG
using Mutex = ProductionMutex;
#else
using Mutex = DebugMutex;
#endif

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#define FETCH_LOCK(lockable) \
  std::lock_guard<decltype(lockable)> FETCH_JOIN(mutex_locked_on_line, __LINE__)(lockable)

}  // namespace mutex
}  // namespace fetch
