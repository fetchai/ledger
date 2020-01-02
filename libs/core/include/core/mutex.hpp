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

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fetch {

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#ifdef FETCH_DEBUG_MUTEX

class DebugMutex;

struct LockLocation
{
  std::string filename{};
  int32_t     line{};
};

class MutexRegister
{
public:
  void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread,
                                LockLocation const &location);
  void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
  void QueueUpFor(DebugMutex *mutex, std::thread::id thread, LockLocation const &location);
  void DeadlockDetected(std::string const &message)
  {
    if (static_cast<bool>(throw_on_deadlock_))
    {
      throw std::runtime_error(message);
    }

    std::cerr << message << std::endl;
    abort();
  }

  static void ThrowOnDeadlock()
  {
    throw_on_deadlock_ = true;
  }

  static void AbortOnDeadlock()
  {
    throw_on_deadlock_ = false;
  }

private:
  void FindDeadlock(DebugMutex *first_mutex, std::thread::id thread, LockLocation const &location);
  std::string CreateTrace(DebugMutex *first_mutex, std::thread::id thread,
                          LockLocation const &location);

  static std::atomic<bool>                          throw_on_deadlock_;
  std::mutex                                        mutex_;
  std::unordered_map<DebugMutex *, std::thread::id> lock_owners_;
  std::unordered_map<std::thread::id, DebugMutex *> waiting_for_;

  std::unordered_map<DebugMutex *, LockLocation>    lock_location_;
  std::unordered_map<std::thread::id, LockLocation> waiting_location_;
};

void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread,
                              LockLocation const &location = LockLocation());
void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread);
void QueueUpFor(DebugMutex *mutex, std::thread::id thread,
                LockLocation const &location = LockLocation());

class DebugMutex : public std::mutex
{
public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  void lock(std::string const &filename, int32_t line)
  {
    LockLocation loc{filename, line};
    QueueUpFor(this, std::this_thread::get_id(), loc);
    std::mutex::lock();
    RegisterMutexAcquisition(this, std::this_thread::get_id(), loc);
  }

  void lock()
  {
    QueueUpFor(this, std::this_thread::get_id());
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
};

template <typename T>
class DebugLockGuard
{
public:
  DebugLockGuard(T &lockable, std::string const & /*filename*/, int32_t /*line*/)
    : lockable_{lockable}
  {
    lockable_.lock();
  }

  ~DebugLockGuard()
  {
    lockable_.unlock();
  }

private:
  T &lockable_;
};

template <>
class DebugLockGuard<DebugMutex>
{
public:
  DebugLockGuard(DebugMutex &lockable, std::string const &filename, int32_t line)
    : lockable_{lockable}
  {
    lockable_.lock(filename, line);
  }

  ~DebugLockGuard()
  {
    lockable_.unlock();
  }

private:
  DebugMutex &lockable_;
};

using Mutex = DebugMutex;

#define FETCH_LOCK(lockable)                                                       \
  fetch::DebugLockGuard<typename std::decay<decltype(lockable)>::type> FETCH_JOIN( \
      mutex_locked_on_line, __LINE__)(lockable, __FILE__, __LINE__)

#else  // !FETCH_DEBUG_MUTEX

using Mutex = std::mutex;

#define FETCH_LOCK(lockable) \
  std::lock_guard<decltype(lockable)> FETCH_JOIN(mutex_locked_on_line, __LINE__)(lockable)

#endif  // FETCH_DEBUG_MUTEX

}  // namespace fetch
