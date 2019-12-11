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

#include "core/mutex.hpp"

#include <iostream>
namespace fetch {
namespace {
Mutexregister mutex_register;
}

void QueueUpFor(DebugMutex *mutex, std::thread::id thread)
{
  mutex_register.QueueUpFor(mutex, thread);
}

void RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread)
{
  mutex_register.RegisterMutexAcquisition(mutex, thread);
}

void UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread)
{
  mutex_register.UnregisterMutexAcquisition(mutex, thread);
}

void FindDeadlock(DebugMutex *mutex, std::thread::id thread)
{
  mutex_register.FindDeadlock(mutex, thread);
}

void Mutexregister::QueueUpFor(DebugMutex *mutex, std::thread::id thread)
{
  std::lock_guard<std::mutex> guard(mutex_);

  FindDeadlock(mutex, thread);
  waiting_for_.insert({thread, mutex});
}

void Mutexregister::RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread)
{
  std::lock_guard<std::mutex> guard(mutex_);

  // Registering the matrix diagonal
  lock_owners_.insert({mutex, thread});
  waiting_for_.erase(thread);
}

void Mutexregister::UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id /*thread*/)
{
  std::lock_guard<std::mutex> guard(mutex_);
  lock_owners_.erase(mutex);
}

void Mutexregister::FindDeadlock(DebugMutex *mutex, std::thread::id thread)
{
  //  std::lock_guard<std::mutex> guard(mutex_);
  auto it = lock_owners_.find(mutex);

  // If the lock is not aquired, we proceed.
  if (it == lock_owners_.end())
  {
    return;
  }

  if (it->second == thread)
  {
    throw std::runtime_error("Simple single mutex recursion.");
  }

  auto next_mutex = mutex;
  while (true)
  {
    auto own_it = lock_owners_.find(next_mutex);
    // All good, nobody is holding a lock on this one.
    if (own_it == lock_owners_.end())
    {
      break;
    }
    auto owner = own_it->second;

    // If we have found a path back to the current thread
    if (owner == thread)
    {
      throw std::runtime_error("Deadlock detected.");
    }

    auto wfit = waiting_for_.find(owner);
    if (wfit == waiting_for_.end())
    {
      break;
    }
    next_mutex = wfit->second;
  }
}

}  // namespace fetch
