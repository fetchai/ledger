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

#define FETCH_DEBUG_MUTEX

#include "core/mutex.hpp"
#include "moment/clocks.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace std::chrono_literals;

namespace fetch {

std::atomic<bool> DeadlockHandler::throw_on_deadlock_{false};

// The default deadlock timeout for recursive mutexes is 40 minutes.
std::atomic<uint64_t> RecursiveLockAttempt::timeout_ms_{2'400'000ull};

/**
 * Either throws a runtime error with a message,
 * or prints that message on stderr and aborts,
 * depending on throw_on_deadlock_ value.
 *
 * @param message Error message for the exception raised/to be printed before abort()
 */
void DeadlockHandler::DeadlockDetected(std::string const &message)
{
  if (static_cast<bool>(throw_on_deadlock_))
  {
    throw std::runtime_error(message);
  }

  std::cerr << message << std::endl;
  abort();
}

void DeadlockHandler::ThrowOnDeadlock()
{
  throw_on_deadlock_ = true;
}

void DeadlockHandler::AbortOnDeadlock()
{
  throw_on_deadlock_ = false;
}

/**
 * Check if there's a deadlock on a simple DebugMutex.
 *
 * @param owner Thread id of mutex's current owner
 * @return true iff attempting to lock() this mutex would result in a deadlock
 */
bool SimpleLockAttempt::IsDeadlocked(LockDetails const &owner) noexcept
{
  return owner.id == std::this_thread::get_id();
}

void RecursiveLockAttempt::SetTimeoutMs(uint64_t timeout_ms)
{
  timeout_ms_ = timeout_ms;
}

/**
 * At the moment of mutex acquisition, update owner id record.
 * The moment of time is recorded, when a thread locks a recursive mutex for the first time,
 * and if after recursive_deadlock_timeout_ that thread has not fully released this mutex yetr,
 * a deadlock is assumed.
 * If this owner has already acquired this mutex, recursion depth is incremented.
 *
 * @param owner A record, either fresh or already known, of a thread that owns a recursive mutex.
 * @return true iff it's the very first, depthwise, acquisition of this mutex by this thread
 */
bool RecursiveLockAttempt::Populate(LockDetails &owner) noexcept
{
  if (owner.recursion_depth++ == 0)
  {
    // This mutex was free before, populate the freshly created table entry.
    owner.taken_at = Now();
    return true;
  }
  return false;
}

/**
 * At the moment of mutex release, update owner id record.
 *
 * @param owner A record of a thread that owns a recursive mutex.
 * @return true iff this thread has just fully released this mutex.
 */
bool RecursiveLockAttempt::Depopulate(LockDetails &owner) noexcept
{
  return --owner.recursion_depth == 0;
}

/**
 * At the moment of a recursive mutex acquisition, check if this mutex can be safely recursively
 * lecked by this thread, i.e. if the thread already has this mutex acquired.
 *
 * @param owner A record of a thread that is to lock a recursive mutex.
 * @return true iff this thread already has this mutex locked.
 */
bool RecursiveLockAttempt::SafeToLock(LockDetails const &owner) noexcept
{
  return owner.id == std::this_thread::get_id();
}

/**
 * Check if there's a deadlock on a RecursiveDebugMutex.
 * In fact, checks if another thread has been holding this mutex for way too long.
 *
 * @param owner A record of a thread that owns a recursive mutex.
 * @return true iff this takes suspiciously long.
 */
bool RecursiveLockAttempt::IsDeadlocked(LockDetails const &owner)
{
  return owner.id != std::this_thread::get_id() &&
         Now() >= owner.taken_at + std::chrono::milliseconds(timeout_ms_);
}

namespace {

auto Clock()
{
  auto ret_val = moment::GetClock("core:RecursiveLockAttempt");
  if (!ret_val)
  {
    throw std::runtime_error("Failed to initialize core:RecursiveLockAttempt clock");
  }
  return ret_val;
}

}  // namespace

RecursiveLockAttempt::Timestamp RecursiveLockAttempt::Now()
{
  static auto clock = Clock();
  return clock->Now();
}

}  // namespace fetch
