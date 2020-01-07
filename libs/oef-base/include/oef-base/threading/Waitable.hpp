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

#include "oef-base/threading/Notification.hpp"
#include <atomic>
#include <mutex>
#include <vector>

namespace fetch {
namespace oef {
namespace base {

class Waitable
{
public:
  using Mutex   = std::mutex;
  using Lock    = std::lock_guard<Mutex>;
  using Waiting = std::vector<Notification::Notification>;

  Waitable()
    : cancelled{false}
    , woken_(false)
  {}
  virtual ~Waitable() = default;

  Notification::NotificationBuilder MakeNotification();
  void                              wake();

  void swap(Waitable &other)
  {
    Lock lock_other(other.mutex);
    Lock lock(mutex);
    std::swap(waiting, other.waiting);
    std::swap(cancelled, other.cancelled);
  }

  void cancel();

  bool IsCancelled() const
  {
    Lock lock(mutex);
    return cancelled;
  }

protected:
  Waiting           waiting;
  bool              cancelled;
  std::atomic<bool> woken_;
  mutable Mutex     mutex;

private:
  Waitable(const Waitable &other) = delete;              // { copy(other); }
  Waitable &operator=(const Waitable &other)  = delete;  // { copy(other); return *this; }
  bool      operator==(const Waitable &other) = delete;  // const { return compare(other)==0; }
  bool      operator<(const Waitable &other)  = delete;  // const { return compare(other)==-1; }
};

void swap(Waitable &v1, Waitable &v2);

}  // namespace base
}  // namespace oef
}  // namespace fetch
