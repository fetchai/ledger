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
#include <condition_variable>
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

class MutexRegister
{
public:
  void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread, LockLocation location)
  {
    FETCH_LOCK(mutex_);

    // Registering the matrix diagonal
    auto lock_owners_it = lock_owners_.emplace(mutex, thread).first;
    try
    {
      lock_location_.emplace(mutex, std::move(location));
      waiting_for_.erase(thread);
      waiting_location_.erase(thread);
    }
    catch (...)
    {
      lock_owners_.erase(lock_owners_it);
      throw;
    }
  }

  void RegisterMutexAcquisition(DebugRecursiveMutex *mutex, std::thread::id thread,
                                LockLocation location)
  {
    FETCH_LOCK(mutex_);

    // Registering the matrix diagonal
    auto rec_lock_owners_it = rec_lock_owners_.emplace(mutex, RecLock{
      thread,

          template <class UnderlyingMutex>
          void UnregisterMutexAcquisition(DebugMutex<UnderlyingMutex> * mutex,
                                          std::thread::id /*thread*/)
      {
        FETCH_LOCK(mutex_);

        if (lock_owners_.erase(mutex) != 0)
        {
          lock_location_.erase(mutex);
        }
      }

      template <class UnderlyingMutex>
      void QueueUpFor(DebugMutex<UnderlyingMutex> * mutex, std::thread::id thread,
                      LockLocation location)
      {
        FETCH_LOCK(mutex_);

        FindDeadlock(mutex, thread, location);
        auto waiting_for_it = waiting_for_.emplace(thread, mutex).first;
        try
        {
          waiting_location_.emplace(thread, std::move(location));
        }
        catch (...)
        {
          waiting_for_.erase(waiting_for_it);
          throw;
        }
      }

      void DeadlockDetected(std::string message)
      {
        if (static_cast<bool>(throw_on_deadlock_))
        {
          throw std::runtime_error(std::move(message));
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
      using TypeErased = void *;

      void FindDeadlock(TypeErased first_mutex, std::thread::id thread,
                        LockLocation const &location);

      std::string CreateTrace(TypeErased first_mutex, std::thread::id thread,
                              LockLocation const &location);

      static std::atomic<bool> throw_on_deadlock_;
      std::mutex               mutex_;

      std::unordered_map<TypeErased, std::thread::id> lock_owners_;
      std::unordered_map<std::thread::id, TypeErased> waiting_for_;

      std::unordered_map<TypeErased, LockLocation>      lock_location_;
      std::unordered_map<std::thread::id, LockLocation> waiting_location_;
};

extern MutexRegister mutex_register;

template <class UnderlyingMutex>
void QueueUpFor(DebugMutex<UnderlyingMutex> *mutex, std::thread::id thread,
                LockLocation location = LockLocation())
{
      mutex_register.QueueUpFor(mutex, thread, std::move(location));
}

template <class UnderlyingMutex>
void RegisterMutexAcquisition(DebugMutex<UnderlyingMutex> *mutex, std::thread::id thread,
                              LockLocation location = LockLocation())
{
      mutex_register.RegisterMutexAcquisition(mutex, thread, std::move(location));
}

template <class UnderlyingMutex>
void UnregisterMutexAcquisition(DebugMutex<UnderlyingMutex> *mutex, std::thread::id thread)
{
      mutex_register.UnregisterMutexAcquisition(mutex, thread);
}

template <class UnderlyingMutex>
class DebugMutex : UnderlyingMutex
{
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

template <>
class DebugMutex<std::recursive_mutex> : std::recursive_mutex
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

      DebugLockGuard(Lockable & lockable, char const * /*filename*/, uint32_t /*line*/)
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

      DebugLockGuard(Lockable & lockable, std::string filename, uint32_t line)
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
