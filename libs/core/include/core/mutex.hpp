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

#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fetch {
class DebugMutex;

class Mutexregister
{
public:
  void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
  void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
  void FindDeadlock(DebugMutex *mutex, std::thread::id thread);

private:
  std::mutex                                                     mutex_;
  std::unordered_map<DebugMutex *, std::thread::id>              lock_owners_;
  std::unordered_map<std::thread::id, std::vector<DebugMutex *>> acquired_locks_;
};

void RegisterMutex(DebugMutex *mutex);
void UnregisterMutex(DebugMutex *mutex);
void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
void FindDeadlock(DebugMutex *mutex, std::thread::id thread);

class DebugMutex : public std::mutex
{
public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  void lock()
  {
    FindDeadlock(this, std::this_thread::get_id());
    std::mutex::lock();
    RegisterMutexAcquisition(this, std::this_thread::get_id());
  }

  void unlock()
  {
    UnregisterMutexAcquisition(this, std::this_thread::get_id());
    std::mutex::unlock();
  }

  bool try_lock()
  {
    if (std::mutex::try_lock())
    {
      RegisterMutexAcquisition(this, std::this_thread::get_id());
      return true;
    }

    return false;
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
  std::string filename_{"unknown"};
  int32_t     line_{0};
};

#ifndef NDEBUG
using Mutex = DebugMutex;
#else
using Mutex = std::mutex;
#endif

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#define FETCH_LOCK(lockable) \
  std::lock_guard<decltype(lockable)> FETCH_JOIN(mutex_locked_on_line, __LINE__)(lockable)

}  // namespace fetch
