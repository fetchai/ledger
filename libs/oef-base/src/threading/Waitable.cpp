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

#include "oef-base/threading/Waitable.hpp"

namespace fetch {
namespace oef {
namespace base {

Notification::NotificationBuilder Waitable::MakeNotification()
{
  Lock lock(mutex);
  auto n = Notification::create();
  waiting.push_back(n);
  return Notification::NotificationBuilder(n, woken_.load());
}

void Waitable::wake()
{
  Waiting waiting_local;
  {
    Lock lock(mutex);
    waiting_local.swap(waiting);
  }

  for (auto &waiter : waiting_local)
  {
    waiter->Notify();
  }
  woken_.store(true);
}

void swap(Waitable &v1, Waitable &v2)
{
  v1.swap(v2);
}

void Waitable::cancel()
{
  Waiting waiting_local;
  {
    Lock lock(mutex);
    cancelled = true;
    waiting_local.swap(waiting);
  }

  for (auto &waiter : waiting_local)
  {
    waiter->Fail();
  }
}

}  // namespace base
}  // namespace oef
}  // namespace fetch
