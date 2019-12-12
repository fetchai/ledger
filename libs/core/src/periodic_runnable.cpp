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

#include "core/periodic_runnable.hpp"

namespace fetch {
namespace core {

PeriodicRunnable::PeriodicRunnable(Duration const &period)
  : last_executed_{Clock::now()}
  , interval_{period}
{}

bool PeriodicRunnable::IsReadyToExecute() const
{
  return (Clock::now() - last_executed_) >= interval_;
}

void PeriodicRunnable::Execute()
{
  // call the periodic function
  Periodically();

  last_executed_ = Clock::now();
}

char const *PeriodicRunnable::GetId() const
{
  return "PeriodicRunnable";
}

}  // namespace core
}  // namespace fetch
