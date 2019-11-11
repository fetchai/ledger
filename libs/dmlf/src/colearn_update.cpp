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

using TimeStamp = ColearnUpdate::TimeStamp;

TimeStamp CurrentTime()
{
  return static_cast<TimeStamp>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch())
                                        .count());
}

}  // namespace

ColearnUpdate::ColearnUpdate(Algorithm algorithm, UpdateType update_type, Data &&data, 
    Source source, Metadata metadata)
  : algorithm_(std::move(algorithm))
  , update_type_{std::move(update_type)}
  , data_{std::move(data)}
  , source_{std::move(source)}
  , time_stamp_{CurrentTime()}
  , metadata_{std::move(metadata)}
{
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
