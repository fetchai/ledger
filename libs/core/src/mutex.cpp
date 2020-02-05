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

#include <iostream>
#include <sstream>

namespace fetch {

#ifdef FETCH_DEBUG_MUTEX

MutexRegister mutex_register;

std::atomic<bool> MutexRegister::throw_on_deadlock_{false};

void MutexRegister::FindDeadlock(TypeErased first_mutex, std::thread::id thread,
                                 LockLocation const &location)
{
  TypeErased mutex = first_mutex;
  while (true)
  {
    auto own_it = lock_owners_.find(mutex);
    // All good, nobody is holding a lock on this one.
    if (own_it == lock_owners_.end())
    {
      break;
    }
    auto owner = own_it->second;

    // If we have found a path back to the current thread
    if (owner == thread)
    {
      DeadlockDetected(CreateTrace(first_mutex, thread, location));
      break;
    }

    auto wfit = waiting_for_.find(owner);
    if (wfit == waiting_for_.end())
    {
      break;
    }

    mutex = wfit->second;
  }
}

std::string MutexRegister::CreateTrace(TypeErased first_mutex, std::thread::id thread,
                                       LockLocation const &location)
{
  std::stringstream ss{""};
  ss << "Deadlock occured when acquiring lock in " << location.filename << ":" << location.line
     << std::endl;

  TypeErased mutex = first_mutex;
  int        n{0};
  while (true)
  {
    auto own_it = lock_owners_.find(mutex);
    // All good, nobody is holding a lock on this one.
    if (own_it == lock_owners_.end())
    {
      ss << "False report - no deadlock." << std::endl;
      return ss.str();
    }

    auto loc1 = lock_location_.find(mutex)->second;
    ss << " - Mutex " << n << " locked in " << loc1.filename << ":" << loc1.line << std::endl;

    auto owner = own_it->second;

    // If we have found a path back to the current thread
    if (owner == thread)
    {
      return ss.str();
    }

    auto wfit = waiting_for_.find(owner);
    if (wfit == waiting_for_.end())
    {
      ss << "False report - no deadlock." << std::endl;
      return ss.str();
    }

    auto loc2 = waiting_location_.find(owner)->second;
    ss << " - Thread " << n << " waits mutex release on " << loc2.filename << ":" << loc2.line
       << std::endl;

    mutex = wfit->second;
    ++n;
  }

  return "This never happened.";
}

#endif  // FETCH_DEBUG_MUTEX

}  // namespace fetch
