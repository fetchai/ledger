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

#include <chrono>

#include "logging/logging.hpp"
#include "oef-base/threading/Task.hpp"

class IKarmaPolicy;

class KarmaRefreshTask : public fetch::oef::base::Task
{
public:
  static constexpr char const *LOGGING_NAME = "KarmaRefreshTask";

  KarmaRefreshTask(IKarmaPolicy *policy, std::size_t interval)
  {
    this->interval = interval;
    this->policy   = policy;
    FETCH_LOG_INFO(LOGGING_NAME, "KarmaRefreshTask CREATED, interval=", interval);
    last_execute = std::chrono::high_resolution_clock::now();
  }
  ~KarmaRefreshTask() override = default;

  bool IsRunnable() const override
  {
    return true;
  }
  fetch::oef::base::ExitState run() override;

protected:
  std::chrono::high_resolution_clock::time_point last_execute;
  IKarmaPolicy *                                 policy;
  std::size_t                                    interval;

private:
  KarmaRefreshTask(const KarmaRefreshTask &other) = delete;
  KarmaRefreshTask &operator=(const KarmaRefreshTask &other)  = delete;
  bool              operator==(const KarmaRefreshTask &other) = delete;
  bool              operator<(const KarmaRefreshTask &other)  = delete;
};
