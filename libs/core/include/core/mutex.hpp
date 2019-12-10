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
#include "logging/logging.hpp"

#include <atomic>
#include <csignal>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>

namespace fetch {
class DebugMutex;

class Mutexregister
{
public:
  void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
  void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
  void FindDeadlock(DebugMutex *mutex, std::thread::id);

private:
  std::mutex                                                     mutex_;
  std::unordered_map<DebugMutex *, std::thread::id>              lock_owners_;
  std::unordered_map<std::thread::id, std::vector<DebugMutex *>> acquired_locks_;
};

void RegisterMutex(DebugMutex *mutex);
void UnregisterMutex(DebugMutex *mutex);
void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
void FindDeadlock(DebugMutex *mutex, std::thread::id);

class DebugMutex
{
public:
  DebugMutex(std::string const &filename, int32_t line)
    : filename_{filename}
    , line_{line}
  {}

  ~DebugMutex()
  {}

  void lock()
  {
    FindDeadlock(this, std::this_thread::get_id());
    mutex_.lock();
    RegisterMutexAcquisition(this, std::this_thread::get_id());
  }

  void unlock()
  {
    UnregisterMutexAcquisition(this, std::this_thread::get_id());
    mutex_.unlock();
  }

  std::lock_guard<DebugMutex> guard()
  {
    return std::lock_guard<DebugMutex>(this);
  }

  std::string filename() const
  {
    return filename_;
  }

  int32_t line() const
  {
    return line_;
  };

private:
  std::mutex  mutex_;
  std::string filename_{};
  int32_t     line_{0};
};

using Mutex = std::mutex;

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#define FETCH_LOCK(lockable) \
  std::lock_guard<decltype(lockable)> FETCH_JOIN(mutex_locked_on_line, __LINE__)(lockable)

}  // namespace fetch
