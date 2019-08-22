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

#include "core/macros.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace fetch {

/**
 * The production mutex is wrapper for the default system mutex.
 */
class ProductionMutex : public std::mutex
{
public:
  ProductionMutex(int, std::string const &);
};

/**
 * The debug mutex acts like a normal mutex but also contains several other checks. This code is
 * intended to be only used in software development.
 */
class DebugMutex : public std::mutex
{
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;
  using Duration  = Clock::duration;

  class MutexTimeout
  {
  public:
    MutexTimeout(std::string filename, int const &line);

    ~MutexTimeout();

    void Eval();

  private:
    std::string       filename_;
    int               line_;
    std::thread       thread_;
    Timepoint         created_ = Clock::now();
    std::atomic<bool> running_{true};
  };

public:
  DebugMutex(int line, std::string file);

  // TODO(ejf) No longer required?
  DebugMutex() = delete;

  DebugMutex &operator=(DebugMutex const &other) = delete;

  void lock();

  void unlock();

  int line() const;

  std::string filename() const;

  std::string AsString();

  std::thread::id thread_id() const;

private:
  std::mutex lock_mutex_;
  Timepoint locked_ FETCH_GUARDED_BY(lock_mutex_);  ///< The time when the mutex was locked
  std::thread::id   thread_id_;                     ///< The last thread to lock the mutex

  int                           line_ = 0;   ///< The line number of the mutex
  std::string                   file_ = "";  ///< The filename of the mutex
  std::unique_ptr<MutexTimeout> timeout_;    ///< The timeout monitor for this mutex
};

#if defined(FETCH_DEBUG_MUTEX) && !(defined(NDEBUG))
using Mutex = DebugMutex;
#else
using Mutex = ProductionMutex;
#endif

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#define FETCH_LOCK(lockable) \
  std::lock_guard<decltype(lockable)> FETCH_JOIN(mutex_locked_on_line, __LINE__)(lockable)

}  // namespace fetch
