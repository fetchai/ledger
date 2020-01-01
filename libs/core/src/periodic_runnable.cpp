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

#include "core/periodic_runnable.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

namespace {
inline std::string ToLowerCase(std::string data)
{
  std::transform(data.begin(), data.end(), data.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return data;
}
}  // namespace

namespace fetch {
namespace core {

PeriodicRunnable::PeriodicRunnable(std::string const &name, Duration const &period)
  : last_executed_{Clock::now()}
  , interval_{period}
  , state_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        ToLowerCase(name) + "_periodic_runnable_gauge",
        "Generic periodic runnable state as integer")}
{}

bool PeriodicRunnable::IsReadyToExecute() const
{
  return (Clock::now() - last_executed_) >= interval_;
}

void PeriodicRunnable::Execute()
{
  // call the periodic function
  state_gauge_->set(static_cast<uint64_t>(1));
  Periodically();
  state_gauge_->set(static_cast<uint64_t>(0));

  last_executed_ = Clock::now();
}

char const *PeriodicRunnable::GetId() const
{
  return "PeriodicRunnable";
}

}  // namespace core
}  // namespace fetch
