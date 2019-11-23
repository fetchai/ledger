#pragma once
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

#include <mutex>

namespace fetch {

//using Mutex = std::mutex;

template <typename UnderlyingMutex>
class CustomMutex : public UnderlyingMutex
{
public:

  CustomMutex(char const *file, int line);
  ~CustomMutex() = default;

  /// @name Mutex Interface
  /// @{
  void lock();
  bool try_lock();
  void unlock();
  /// @}

  char const *file() const
  {
    return file_;
  }

  int line() const
  {
    return line_;
  }

private:

  enum class Event
  {
    WAIT_FOR_LOCK = 0,
    LOCKED,
    UNLOCKED
  };

  /// @name Helpers
  /// @{
  static char const *ToString(Event event);
  void RecordEvent(Event event);
  /// @}

  char const *const file_{nullptr};
  int               line_{0};
};

using ConditionVariable = std::condition_variable;
using Mutex             = CustomMutex<std::mutex>;
using RecursiveMutex    = CustomMutex<std::recursive_mutex>;

#define FETCH_JOIN_IMPL(x, y) x##y
#define FETCH_JOIN(x, y) FETCH_JOIN_IMPL(x, y)

#define FETCH_LOCK(lockable) \
  std::lock_guard<decltype(lockable)> FETCH_JOIN(mutex_locked_on_line, __LINE__)(lockable)

}  // namespace fetch
