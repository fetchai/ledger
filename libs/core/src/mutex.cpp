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

namespace fetch {

#ifdef FETCH_DEBUG_MUTEX

namespace {

moment::ClockInterface::Timestamp Now()
{
  static auto clock = moment::GetClock("RecursiveDeadlock");

  return clock->Now();
}

}  // namespace

std::atomic<bool> DeadlockHandler::throw_on_deadlock_{false};

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

bool SimpleDeadlock::IsDeadlocked(OwnerId const &owner) noexcept
{
  return owner.id == std::this_thread::get_id();
}

bool RecursiveDeadlock::Populate(OwnerId &owner) noexcept
{
  if (owner.recursion_depth++ == 0)
  {
    owner.taken_at = Now();
    return true;
  }
  return false;
}

bool RecursiveDeadlock::Depopulate(OwnerId &owner) noexcept
{
  return --owner.recursion_depth == 0;
}

bool RecursiveDeadlock::SafeToLock(OwnerId const &owner) noexcept
{
  return owner.id == std::this_thread::get_id();
}

bool RecursiveDeadlock::IsDeadlocked(OwnerId const &owner)
{
  static constexpr std::chrono::seconds deadlock_timeout{2};

  return owner.id != std::this_thread::get_id() && Now() >= owner.taken_at + deadlock_timeout;
}

#endif  // FETCH_DEBUG_MUTEX

}  // namespace fetch
