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

#include "core/mutex.hpp"
#include "moment/clocks.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std::chrono_literals;

namespace fetch {

#ifdef FETCH_DEBUG_MUTEX

namespace {

moment::ClockInterface::Timestamp Now()
{
  static auto clock = moment::GetClock("RecursiveLockAttempt");

  return clock->Now();
}

static constexpr moment::ClockInterface::Duration DEADLOCK_TIMEOUT = 2s;

}  // namespace

std::atomic<bool> DeadlockHandler::throw_on_deadlock_{false};

/**
 * Either throws a runtime error with a message,
 * or prints that message on stderr and aborts,
 * depending on throw_on_deadlock_ value.
 *
 * @param message Error message for the exception raised/to be printed before abort()
 */
void DeadlockHandler::DeadlockDetected(std::string message)
{
  if (static_cast<bool>(throw_on_deadlock_))
  {
    throw std::runtime_error(std::move(message));
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
bool SimpleLockAttempt::IsDeadlocked(OwnerId const &owner) noexcept
{
  return owner.id == std::this_thread::get_id();
}

/**
 * At the moment of mutex acquisition, update owner id record.
 * The moment of time is recorded, when a thread locks a recursive mutex for the first time,
 * and if after DEADLOCK_TIMEOUT that thread has not fully released this mutex yetr,
 * a deadlock is assumed.
 * If this owner has already acquired this mutex, recursion depth is incremented.
 *
 * @param owner A record, either fresh or already known, of a thread that owns a recursive mutex.
 * @return true iff it's the very first, depthwise, acquisition of this mutex by this thread
 */
bool RecursiveLockAttempt::Populate(OwnerId &owner) noexcept
{
  if (owner.recursion_depth++ == 0)
  {
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
bool RecursiveLockAttempt::Depopulate(OwnerId &owner) noexcept
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
bool RecursiveLockAttempt::SafeToLock(OwnerId const &owner) noexcept
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
bool RecursiveLockAttempt::IsDeadlocked(OwnerId const &owner)
{
  return owner.id != std::this_thread::get_id() && Now() >= owner.taken_at + DEADLOCK_TIMEOUT;
}

#endif  // FETCH_DEBUG_MUTEX

}  // namespace fetch
