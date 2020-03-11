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

#include "core/macros.hpp"
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

using namespace std::chrono_literals;

namespace fetch {

#ifdef FETCH_DEBUG_MUTEX

/**
 * The two policies describe specific actions to be taken when locking each of the mutex types.
 * They both implement static interface LockPolicy which specifies the following member types:
 *
 * Mutex       — the type of mutex governed by this policy
 *               Mutex inherits this policy, as is common with policies since MC++D
 * LockDetails — value to be kept in lock table, describing current owner thread and line of code
 *               this mutex was acquired at
 *
 * and the following member functions:
 *
 * bool Populate(LockDetails &owner)
 *     extra initialization applied to this mutex's lock table entry when this mutex is locked
 *     returns true iff this mutex has been just locked, after being free
 *
 * bool Depopulate(LockDetails &ex_owner)
 *     extra cleanup applied to this mutex's lock table entry when this mutex is released
 *     returns true iff this mutex has been fully unlocked and is now free
 *
 * bool SafeToLock(LockDetails const &new_owner)
 *     returns true iff this mutex, while being locked, can still be safely locked by new_owner
 *
 * bool IsDeadlocked(LockDetails const &new_owner)
 *     returns true if attempting to lock this mutex by new_owner woudl result in a deadlock
 *
 * template<class Mutex>
 * void Lock(Mutex &mutex, LockLocation const &location)
 *     mutex is locked from location (if there's no deadlock)
 */
class SimpleLockAttempt;
class RecursiveLockAttempt;

/**
 * MutexRegister<LockPolicy> is a table that keeps track of locations where
 * LockPolicy-inheriting mutexes were locked from
 */
template <class LockPolicy>
class MutexRegister;

using SimpleMutexRegister    = MutexRegister<SimpleLockAttempt>;
using RecursiveMutexRegister = MutexRegister<RecursiveLockAttempt>;

/**
 * The DebugMutex class decorates UnderlyingMutex by registering locks/unlocks with MutexRegister.
 */
template <class UnderlyingMutex, class MutexRegister>
class DebugMutex;

using SimpleDebugMutex    = DebugMutex<std::mutex, SimpleLockAttempt>;
using RecursiveDebugMutex = DebugMutex<std::recursive_timed_mutex, RecursiveLockAttempt>;

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
  static void DeadlockDetected(std::string const &message);

  // Setters
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

  // LockPolicy's methods are trivial for non-recursive mutexes.
  static constexpr bool Populate(LockDetails & /*owner*/) noexcept
  {
    return true;
  }

  static constexpr bool Depopulate(LockDetails & /*owner*/) noexcept
  {
    return true;
  }

  // It is never safe to try to lock a locked non-recursive mutex, even in the same thread.
  static constexpr bool SafeToLock(LockDetails const & /*owner*/) noexcept
  {
    return false;
  }

  static bool IsDeadlocked(LockDetails const &owner) noexcept;

  template <class M>
  static void Lock(M &m, LockLocation const & /*location*/)
  {
    m.lock();
  }
};

class RecursiveLockAttempt
{
public:
  static void SetTimeoutMs(uint64_t timeout_ms);

protected:
  using Mutex    = RecursiveDebugMutex;
  using Duration = moment::ClockInterface::Duration;

  struct LockDetails
  {
    moment::ClockInterface::Timestamp taken_at;
    std::thread::id                   id              = std::this_thread::get_id();
    uint64_t                          recursion_depth = 0;
  };

  static bool Populate(LockDetails &owner) noexcept;

  static bool Depopulate(LockDetails &owner) noexcept;

  static bool SafeToLock(LockDetails const &owner) noexcept;

  static bool IsDeadlocked(LockDetails const &owner);

  template <class M>
  void Lock(M &m, LockLocation const &location)
  {
    uint64_t current_score = locked_times_;
    // Keep attempting to lock the mutex for specified time.
    while (!m.try_lock_for(std::chrono::milliseconds(timeout_ms_)))
    {
      // The mutex is still locked by someone else.
      if (locked_times_ == current_score)
      {
        // During the call to try_lock_for(), mutex's history of locks hasn't changed.
        // Disregarding the possibility that it was locked by other threads 2⁶⁴ times
        // while we were waiting, this should mean the same thread is still holding it
        // for way too long.
        std::stringstream ss{""};
        ss << "Deadlock occured when acquiring lock at " << location.filename << ":"
           << location.line << std::endl;
        DeadlockHandler::DeadlockDetected(ss.str());
        // Control cannot reach this line.
      }
      // Update the score and keep trying to lock it.
      current_score = locked_times_;
    }
    // Update mutex's history of locks.
    ++locked_times_;
  }

private:
  static std::atomic<uint64_t>
                        timeout_ms_;       // Signal deadlock if a thread holds a mutex for so long.
  std::atomic<uint64_t> locked_times_{0};  // History of locks.
};

/**
 * MutexRegister maintains mutex-thread-source code position relations.
 */
template <class LockPolicy>
class MutexRegister : public DeadlockHandler, LockPolicy
{
  static_assert(type_util::IsAnyOfV<LockPolicy, SimpleLockAttempt, RecursiveLockAttempt>,
                "Only DebugMutex and RecursiveDebugMutex can be tracked as of now");

  using Parent = LockPolicy;
  using typename Parent::LockDetails;
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

    // Create of find a table record for this mutex.
    auto lock_owners_it = instance.lock_owners_.emplace(mutex, LockDetails{}).first;
    try
    {
      // Initialize/update table record.
      if (Populate(lock_owners_it->second))
      {
        // Populate() returned true; this indicates the mutex has been acquired for the first time
        // so we create a source code position table entry.
        instance.lock_location_.emplace(mutex, std::move(location));
      }
    }
    catch (...)
    {
      // cleanup for exception guarantees
      instance.lock_owners_.erase(lock_owners_it);
      throw;
    }

    // This thread has gained that mutex; deregister its waiting-for status.
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
      // Depopulate() returned true, which means the mutex has ben released completely
      // and is now free.
      // Erase table entries.
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

    // Deadlock not detected, register this thread's waiting-for status.
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
      // Either this mutex is still free, or it's safe to lock.
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
      // Check if that thread is in turn waiting for some other mutex.
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
   * @param location Code point the attempted lock is performed at
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

  std::unordered_map<MutexPtr, LockDetails>     lock_owners_;
  std::unordered_map<std::thread::id, MutexPtr> waiting_for_;

  std::unordered_map<MutexPtr, LockLocation>        lock_location_;
  std::unordered_map<std::thread::id, LockLocation> waiting_location_;
};

using SimpleMutexRegister    = MutexRegister<SimpleLockAttempt>;
using RecursiveMutexRegister = MutexRegister<RecursiveLockAttempt>;

template <class UnderlyingMutex, class LockPolicy>
class DebugMutex : UnderlyingMutex, LockPolicy
{
  using Register = MutexRegister<LockPolicy>;

public:
  DebugMutex()  = default;
  ~DebugMutex() = default;

  /**
   * Locks this mutex at specified source code location.
   *
   * @param loc
   */
  void lock(LockLocation loc)
  {
    Register::QueueUpFor(this, loc);
    LockPolicy::Lock(Unsafe(), loc);
    Register::RegisterMutexAcquisition(this, std::move(loc));
  }

  /**
   * STL-conformat lock(), locks this mutex the same way as above, location remains unknown.
   */
  void lock()
  {
    lock({});
  }

  void unlock()
  {
    Register::UnregisterMutexAcquisition(this);
    UnderlyingMutex::unlock();
  }

  bool try_lock()
  {
    if (UnderlyingMutex::try_lock())
    {
      Register::RegisterMutexAcquisition(this);
      return true;
    }

    return false;
  }

private:
  /**
   * Get the underlying mutex for direct locking.
   *
   * @return
   */
  UnderlyingMutex &Unsafe()
  {
    return *this;
  }
};

/**
 * RAII-locker, similar to std::lock_guard, but source code position-aware.
 */
template <typename M>
class DebugLockGuard
{
public:
  using Lockable = M;

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

/**
 * Only with decorated mutex types defined in this header, source code position-awareness matters.
 */
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

#define FETCH_LOCK(lockable)                                          \
  fetch::DebugLockGuard<std::decay_t<decltype(lockable)>> FETCH_JOIN( \
      mutex_locked_on_line, __LINE__)(lockable, __FILE__, __LINE__)

#else  // !FETCH_DEBUG_MUTEX

#define FETCH_LOCK(lockable)                                                         \
  std::lock_guard<std::decay_t<decltype(lockable)>> FETCH_JOIN(mutex_locked_on_line, \
                                                               __LINE__)(lockable)

using Mutex             = std::mutex;
using RMutex            = std::recursive_mutex;
using ConditionVariable = std::condition_variable;

#endif  // FETCH_DEBUG_MUTEX

}  // namespace fetch
