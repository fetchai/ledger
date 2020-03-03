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

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
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

class SimpleDeadlock;
class RecursiveDeadlock;

template <class DeadlockPolicy>
class MutexRegister;

using SimpleMutexRegister    = MutexRegister<SimpleDeadlock>;
using RecursiveMutexRegister = MutexRegister<RecursiveDeadlock>;

template <class UnderlyingMutex, class MutexRegister>
class DebugMutex;

using SimpleDebugMutex    = DebugMutex<std::mutex, SimpleMutexRegister>;
using RecursiveDebugMutex = DebugMutex<std::recursive_mutex, RecursiveMutexRegister>;

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
  static void DeadlockDetected(std::string message);

  static void ThrowOnDeadlock();

  static void AbortOnDeadlock();

private:
  static std::atomic<bool> throw_on_deadlock_;
};

class SimpleLockAttempt
{
protected:
  using Mutex = SimpleDebugMutex;

  struct LockDetails
  {
    std::thread::id id = std::this_thread::get_id();
  };

  static constexpr bool Populate(OwnerId & /*owner*/) noexcept
  {
    return true;
  }

  static constexpr bool Depopulate(OwnerId & /*owner*/) noexcept
  {
    return true;
  }

  static constexpr bool SafeToLock(OwnerId const & /*owner*/) noexcept
  {
    return false;
  }

  static bool IsDeadlocked(OwnerId const &owner) noexcept;
};

class RecursiveLockAttempt
{
protected:
  using Mutex = RecursiveDebugMutex;

  struct LockDetails
  {
    moment::ClockInterface::Timestamp taken_at;
    std::thread::id                   id              = std::this_thread::get_id();
    uint64_t                          recursion_depth = 0;
  };

  static bool Populate(OwnerId &owner) noexcept;

  static bool Depopulate(OwnerId &owner) noexcept;

  static bool SafeToLock(OwnerId const &owner) noexcept;

  static bool IsDeadlocked(OwnerId const &owner);
};

template <class DeadlockPolicy>
class MutexRegister : public DeadlockHandler, DeadlockPolicy
{
  static_assert(type_util::IsAnyOfV<DeadlockPolicy, SimpleDeadlock, RecursiveDeadlock>,
                "Only DebugMutex and DebugRecursiveMutex can be tracked as of now");

  using Parent = DeadlockPolicy;
  using typename Parent::OwnerId;
  using MutexPtr = typename Parent::Mutex *;

  using Parent::Depopulate;
  using Parent::Populate;

public:
  /**
   * Once a thread locked a mutex successfully, update the tables.
   *
   * @param mutex Pointer to the mutex locked
   * @param location Code point the lock is performed at
   */
  static void RegisterMutexAcquisition(MutexPtr mutex, LockLocation location = LockLocation())
  {
    auto &         instance = Instance();
    GuardOfMutexes non_fetch_lock_to_avoid_recursion(instance.mutex_);

    auto lock_owners_it = instance.lock_owners_.emplace(mutex, OwnerId{}).first;
    try
    {
      if (Populate(lock_owners_it->second))
      {
        instance.lock_location_.emplace(mutex, std::move(location));
      }
    }
    catch (...)
    {
      // cleanup for exception guarantees
      instance.lock_owners_.erase(lock_owners_it);
      throw;
    }

    auto thread = std::this_thread::get_id();
    instance.waiting_for_.erase(thread);
    instance.waiting_location_.erase(thread);
  }

  /**
   * Before a thread fully releases a mutex, update the tables.
   *
   * @param mutex A pointer to the mutex to be released
   */
  static void UnregisterMutexAcquisition(MutexPtr mutex)
  {
    auto &         instance = Instance();
    GuardOfMutexes non_fetch_lock_to_avoid_recursion(instance.mutex_);

    auto lock_owners_it = instance.lock_owners_.find(mutex);
    assert(lock_owners_it != instance.lock_owners_.end());

    if (Depopulate(lock_owners_it->second))
    {
      instance.lock_owners_.erase(lock_owners_it);
      instance.lock_location_.erase(mutex);
    }
  }

  /**
   * Announce that a thread is after a mutex.
   *
   * @param mutex A pointer to a mutex to be locked
   * @param location Code point the lock is performed at
   */
  static void QueueUpFor(MutexPtr mutex, LockLocation location)
  {
    auto &         instance = Instance();
    GuardOfMutexes non_fetch_lock_to_avoid_recursion(instance.mutex_);

    instance.CheckForDeadlocks(mutex, location);

    auto thread         = std::this_thread::get_id();
    auto waiting_for_it = instance.waiting_for_.emplace(thread, mutex).first;
    try
    {
      instance.waiting_location_.emplace(thread, std::move(location));
    }
    catch (...)
    {
      // cleanup for exception guarantees
      instance.waiting_for_.erase(waiting_for_it);
      throw;
    }
  }

private:
  using MutexOfMutexes = std::mutex;
  using GuardOfMutexes = std::lock_guard<MutexOfMutexes>;

  static MutexRegister &Instance()
  {
    static MutexRegister instance;
    return instance;
  }

  using Parent::IsDeadlocked;
  using Parent::SafeToLock;

  /**
   * Check if there's a deadlock in the current lock graph.
   *
   * @param mutex Pointer to the mutex to be locked
   * @param location Code point the lock is performed at
   */
  void CheckForDeadlocks(MutexPtr mutex, LockLocation const &location)
  {
    auto own_it = lock_owners_.find(mutex);
    if (own_it == lock_owners_.end() || SafeToLock(own_it->second))
    {
      return;
    }
    while (own_it != lock_owners_.end())
    {
      auto const &owner = own_it->second;
      if (IsDeadlocked(owner))
      {
        DeadlockDetected(CreateTrace(mutex, location));
        return;
      }
      auto wfit = waiting_for_.find(owner.id);
      if (wfit == waiting_for_.end())
      {
        return;
      }
      mutex  = wfit->second;
      own_it = lock_owners_.find(mutex);
    }
  }

  /**
   * When a deadlock detected, create an error message to be thrown/printed to stderr.
   *
   * @param mutex A pointer to mutex locked deadly
   * @param location Code point the lock is performed at
   */
  std::string CreateTrace(MutexPtr mutex, LockLocation const &location)
  {
    std::stringstream ss{""};
    ss << "Deadlock occured when acquiring lock at " << location.filename << ":" << location.line
       << std::endl;

    int  n{0};
    auto own_it = lock_owners_.find(mutex);
    if (own_it == lock_owners_.end() || SafeToLock(own_it->second))
    {
      ss << "False report — no deadlock." << std::endl;
      return ss.str();
    }

    while (true)
    {
      auto loc1 = lock_location_.find(mutex)->second;
      ss << " - Mutex " << n << " locked at " << loc1.filename << ":" << loc1.line << std::endl;

      auto owner = own_it->second;

      // If we have found a path back to the current thread
      if (IsDeadlocked(owner))
      {
        return ss.str();
      }

      auto wfit = waiting_for_.find(owner.id);
      if (wfit == waiting_for_.end())
      {
        ss << "False report — no deadlock." << std::endl;
        return ss.str();
      }

      auto loc2 = waiting_location_.find(owner.id)->second;
      ss << " - Thread " << n << " awaits mutex release at " << loc2.filename << ":" << loc2.line
         << std::endl;

      mutex  = wfit->second;
      own_it = lock_owners_.find(mutex);
      if (own_it == lock_owners_.end())
      {
        ss << "False report — no deadlock." << std::endl;
        return ss.str();
      }
      ++n;
    }

    return "This never happened.";
  }

  MutexOfMutexes mutex_;

  std::unordered_map<MutexPtr, OwnerId>         lock_owners_;
  std::unordered_map<std::thread::id, MutexPtr> waiting_for_;

  std::unordered_map<MutexPtr, LockLocation>        lock_location_;
  std::unordered_map<std::thread::id, LockLocation> waiting_location_;
};

using SimpleMutexRegister    = MutexRegister<SimpleLockAttempt>;
using RecursiveMutexRegister = MutexRegister<RecursiveLockAttempt>;

template <class UnderlyingMutex, class MutexRegister>
class DebugMutex : UnderlyingMutex
{
public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  void lock(LockLocation loc)
  {
    MutexRegister::QueueUpFor(this, loc);
    UnderlyingMutex::lock();
    MutexRegister::RegisterMutexAcquisition(this, std::move(loc));
  }

  void lock()
  {
    lock({});
  }

  void unlock()
  {
    MutexRegister::UnregisterMutexAcquisition(this);
    UnderlyingMutex::unlock();
  }

  bool try_lock()
  {
    if (UnderlyingMutex::try_lock())
    {
      MutexRegister::RegisterMutexAcquisition(this);
      return true;
    }

    return false;
  }
};

using SimpleDebugMutex    = DebugMutex<std::mutex, SimpleMutexRegister>;
using RecursiveDebugMutex = DebugMutex<std::recursive_mutex, RecursiveMutexRegister>;

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

template <class UnderlyingMutex, class MutexRegister>
class DebugLockGuard<DebugMutex<UnderlyingMutex, MutexRegister>>
{
public:
  using Lockable = DebugMutex<UnderlyingMutex, MutexRegister>;

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

using Mutex             = SimpleDebugMutex;
using RMutex            = RecursiveDebugMutex;
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
