#include "core/mutex.hpp"

namespace fetch {
namespace {
Mutexregister mutex_register;
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

void Mutexregister::RegisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread)
{
  std::lock_guard<std::mutex> guard(mutex_);
  lock_owners_.insert({mutex, thread});

  auto it = acquired_locks_.find(thread);

  if (it == acquired_locks_.end())
  {
    acquired_locks_.insert({thread, std::vector<DebugMutex *>()});
    it = acquired_locks_.find(thread);
  }

  it->second.push_back(mutex);
}

void Mutexregister::UnregisterMutexAcquisition(DebugMutex *mutex, std::thread::id thread)
{
  std::lock_guard<std::mutex> guard(mutex_);
  lock_owners_.erase(mutex);

  auto it = acquired_locks_.find(thread);
  it->second.pop_back();
}

void Mutexregister::FindDeadlock(DebugMutex *mutex, std::thread::id thread)
{
  std::lock_guard<std::mutex> guard(mutex_);
  auto                        it = lock_owners_.find(mutex);

  // If the lock is not aquired, we proceed.
  if (it == lock_owners_.end())
  {
    return;
  }

  if (it->second == thread)
  {
    throw std::runtime_error("Simple single mutex recursion.");
  }

  auto mutices = acquired_locks_.find(thread)->second;
  for (auto &m : mutices)
  {
    if (m == mutex)
    {
      throw std::runtime_error("Single thread, multi-mutex recursion.");
    }
  }

  // TODO: Handle mutli thread, multi mutex case
}

}  // namespace fetch