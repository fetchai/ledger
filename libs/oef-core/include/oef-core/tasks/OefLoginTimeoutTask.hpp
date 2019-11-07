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

#include <chrono>

#include "logging/logging.hpp"
#include "oef-base/threading/Task.hpp"

class OefAgentEndpoint;

class OefLoginTimeoutTask : public Task
{
public:
  static constexpr char const *LOGGING_NAME = "OefLoginTimeoutTask";

  explicit OefLoginTimeoutTask(const std::shared_ptr<OefAgentEndpoint> &ep)
  {
    this->ep = ep;
  }
  ~OefLoginTimeoutTask() override = default;

  bool IsRunnable() const override
  {
    return true;
  }
  ExitState run() override;

protected:
  std::weak_ptr<OefAgentEndpoint> ep;

private:
  OefLoginTimeoutTask(const OefLoginTimeoutTask &other) = delete;
  OefLoginTimeoutTask &operator=(const OefLoginTimeoutTask &other)  = delete;
  bool                 operator==(const OefLoginTimeoutTask &other) = delete;
  bool                 operator<(const OefLoginTimeoutTask &other)  = delete;
};
