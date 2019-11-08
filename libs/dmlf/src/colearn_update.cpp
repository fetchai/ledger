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

#include "dmlf/colearn/colearn_update.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

namespace {

using TimeStampType = ColearnUpdate::TimeStampType;

TimeStampType CurrentTime()
{
  return static_cast<TimeStampType>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count());
}

}

ColearnUpdate::ColearnUpdate(std::string updateType, Data&& data, Source source)
: update_type{std::move(updateType)}, data{std::move(data)}
, time_stamp{CurrentTime()}, source{std::move(source)}
{
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
