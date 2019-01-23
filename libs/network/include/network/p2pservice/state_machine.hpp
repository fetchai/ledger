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

#include <unordered_map>
#include <vector>

#include "core/mutex.hpp"
#include "network/service/promise.hpp"

namespace fetch {
namespace p2p {

template <typename State>
class StateMachine
{
public:
  using Promise     = service::Promise;
  using PromiseList = std::vector<Promise>;

  StateMachine()  = default;
  ~StateMachine() = default;

  bool AddState(State const &state)
  {
    auto const result = map_.insert(state);
    return result.second;
  }

private:
  using StateMap = std::unordered_map<State, Promise>;
  using Mutex    = mutex::Mutex;

  StateMap map_;
};

}  // namespace p2p
}  // namespace fetch
