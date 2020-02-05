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

#include "meta/type_util.hpp"
#include "moment/clock_interfaces.hpp"
#include "moment/clocks.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace fetch {

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#ifdef FETCH_DEBUG_MUTEX

#define FETCH_LOCK(lockable)                                          \
  fetch::DebugLockGuard<std::decay_t<decltype(lockable)>> FETCH_JOIN( \
      mutex_locked_on_line, __LINE__)(lockable, __FILE__, __LINE__)

class DebugMutex;
class DebugRecursiveMutex;

template <typename Mutex>
class DebugLockGuard;

struct LockLocation
{
  std::string filename;
  uint32_t    line{};
};

class DeadlockHandler
{
public:
  void DeadlockDetected(std::string message);

  static void ThrowOnDeadlock();

  static void AbortOnDeadlock();

private:
  static std::atomic<bool> throw_on_deadlock_{};
};

class MutexRegister : public DeadlockHandler
{
public:
  void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread, LockLocation location);

  void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id /*thread*/);

  void QueueUpFor(DebugMutex *mutex, std::thread::id thread, LockLocation location);

private:
  void FindDeadlock(DebugMutex *first_mutex, std::thread::id thread, LockLocation const &location);

  std::string CreateTrace(DebugMutex *first_mutex, std::thread::id thread,
                          LockLocation const &location);

  std::mutex mutex_;

  std::unordered_map<DebugMutex *, std::thread::id> lock_owners_;
  std::unordered_map<std::thread::id, DebugMutex *> waiting_for_;

  std::unordered_map<DebugMutex *, LockLocation>    lock_location_;
  std::unordered_map<std::thread::id, LockLocation> waiting_location_;
};

class RecursiveMutexRegister : public DeadlockHandler
{
public:
  void RegisterMutexAcquisition(DebugRecursiveMutex *mutex, std::thread::id thread,
                                LockLocation location);

  void UnregisterMutexAcquisition(DebugRecursiveMutex *mutex, std::thread::id /*thread*/);

  void QueueUpFor(DebugRecursiveMutex *mutex, std::thread::id thread, LockLocation location);

private:
  struct OwnerRecord
  {
    std::thread::id                   thread;
    moment::ClockInterface::Timestamp taken_at;
    uint64_t                          recursion_depth;
  };

  void FindDeadlock(DebugRecursiveMutex *first_mutex, std::thread::id thread,
                    LockLocation const &location);

  std::string CreateTrace(DebugRecursiveMutex *first_mutex, std::thread::id thread,
                          LockLocation const &location);

  std::mutex       mutex_;
  moment::ClockPtr clock_ = moment::GetClock("RecursiveMutexRegister");

  std::unordered_map<DebugRecursiveMutex *, OwnerRecord>     lock_owners_;
  std::unordered_map<std::thread::id, DebugRecursiveMutex *> waiting_for_;

  std::unordered_map<DebugRecursiveMutex *, LockLocation> lock_location_;
  std::unordered_map<std::thread::id, LockLocation>       waiting_location_;
};

template <class Mutex>
std::conditional_t<std::is_same<Mutex, DebugMutex>::value, MutexRegister, RecursiveMutexRegister>
    &ProperMutexRegister()
{
  static_assert(type_util::IsAnyOfV<Mutex, DebugMutex, DebugRecursiveMutex>, "Unknown mutex type");

  using RetVal = std::conditional_t<std::is_same<Mutex, DebugMutex>::value, MutexRegister,
                                    RecursiveMutexRegister>;

  static RetVal ret_val;
  return ret_val;
}

template <class Mutex>
void QueueUpFor(Mutex *mutex, std::thread::id thread, LockLocation location = LockLocation())
{
  ProperMutexRegister<Mutex>().QueueUpFor(mutex, thread, std::move(location));
}

template <class Mutex>
void RegisterMutexAcquisition(Mutex *mutex, std::thread::id thread,
                              LockLocation location = LockLocation())
{
  ProperMutexRegister<Mutex>().RegisterMutexAcquisition(mutex, thread, std::move(location));
}

template <class Mutex>
void UnregisterMutexAcquisition(Mutex *mutex, std::thread::id thread)
{
  ProperMutexRegister<Mutex>().UnregisterMutexAcquisition(mutex, thread);
}

class DebugMutex : std::mutex
{
  using UnderlyingMutex = std::mutex;

public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  void lock(LockLocation loc)
  {
    QueueUpFor(this, std::this_thread::get_id(), loc);
    UnderlyingMutex::lock();
    RegisterMutexAcquisition(this, std::this_thread::get_id(), std::move(loc));
  }

  void lock()
  {
    lock({});
  }

  void unlock()
  {
    UnregisterMutexAcquisition(this, std::this_thread::get_id());
    UnderlyingMutex::unlock();
  }

  bool try_lock()
  {
    if (UnderlyingMutex::try_lock())
    {
      RegisterMutexAcquisition(this, std::this_thread::get_id());
      return true;
    }

    return false;
  }
};

class DebugRecursiveMutex : std::recursive_mutex
{
  using UnderlyingMutex = std::recursive_mutex;

public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  void lock(LockLocation loc)
  {
    bool needs_reg = recursion_depth_ == 0;

    if (needs_reg)
    {
      QueueUpFor(this, std::this_thread::get_id(), loc);
    }

    UnderlyingMutex::lock();
    ++recursion_depth_;

    if (needs_reg)
    {
      RegisterMutexAcquisition(this, std::this_thread::get_id(), std::move(loc));
    }
  }

  void lock()
  {
    lock({});
  }

  void unlock()
  {
    if (recursion_depth_-- == 1)
    {
      UnregisterMutexAcquisition(this, std::this_thread::get_id());
    }
    UnderlyingMutex::unlock();
  }

  bool try_lock()
  {
    if (UnderlyingMutex::try_lock())
    {
      if (++recursion_depth_ == 1)
      {
        RegisterMutexAcquisition(this, std::this_thread::get_id());
      }
      return true;
    }

    return false;
  }

private:
  uint64_t recursion_depth_{};
};

template <typename T>
class DebugLockGuard
{
public:
  using Lockable = T;

  DebugLockGuard(Lockable &lockable, char const * /*filename*/, uint32_t /*line*/)
    : lockable_{lockable}
  {
    lockable_.lock();
  }

  ~DebugLockGuard()
  {
    lockable_.unlock();
  }

private:
  Lockable &lockable_;
};

template <class UnderlyingMutex>
class DebugLockGuard<DebugMutex<UnderlyingMutex>>
{
public:
  using Lockable = DebugMutex<UnderlyingMutex>;

  DebugLockGuard(Lockable &lockable, std::string filename, uint32_t line)
    : lockable_{lockable}
  {
    lockable_.lock(LockLocation{std::move(filename), line});
  }

  ~DebugLockGuard()
  {
    lockable_.unlock();
  }

private:
  Lockable &lockable_;
};

using Mutex             = DebugMutex<std::mutex>;
using RMutex            = DebugMutex<std::recursive_mutex>;
using ConditionVariable = std::condition_variable_any;

#else  // !FETCH_DEBUG_MUTEX

#define FETCH_LOCK(lockable)                                                         \
  std::lock_guard<std::decay_t<decltype(lockable)>> FETCH_JOIN(mutex_locked_on_line, \
                                                               __LINE__)(lockable)

using Mutex             = std::mutex;
using RMutex            = std::recursive_mutex;
using ConditionVariable = std::condition_variable;

#endif  // FETCH_DEBUG_MUTEX

}  // namespace fetch
