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

#include "dmlf/colearn/muddle_message_task.hpp"
#include "dmlf/colearn/update_store.hpp"

namespace fetch {
namespace colearn {

class MuddleUpdateTask : public MuddleMessageTask
{
public:
  MuddleUpdateTask()           = default;
  ~MuddleUpdateTask() override = default;

  MuddleUpdateTask(MuddleUpdateTask const &other) = delete;
  MuddleUpdateTask &operator=(MuddleUpdateTask const &other)  = delete;
  bool              operator==(MuddleUpdateTask const &other) = delete;
  bool              operator<(MuddleUpdateTask const &other)  = delete;

protected:
private:
};

}  // namespace colearn
}  // namespace fetch
