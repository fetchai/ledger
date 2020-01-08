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

#include "logging/logging.hpp"
#include "oef-base/threading/Task.hpp"

namespace fetch {
namespace oef {
namespace base {

class MonitoringTask : public Task
{
public:
  static constexpr char const *LOGGING_NAME = "MonitoringTask";

  MonitoringTask()           = default;
  ~MonitoringTask() override = default;

  bool IsRunnable() const override
  {
    return true;
  }
  ExitState run() override;

protected:
private:
  MonitoringTask(const MonitoringTask &other) = delete;  // { copy(other); }
  MonitoringTask &operator                    =(const MonitoringTask &other) =
      delete;                                             // { copy(other); return *this; }
  bool operator==(const MonitoringTask &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const MonitoringTask &other)  = delete;  // const { return compare(other)==-1; }
};

}  // namespace base
}  // namespace oef
}  // namespace fetch
