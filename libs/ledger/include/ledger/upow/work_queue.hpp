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

#include "ledger/upow/work.hpp"

#include <queue>

namespace fetch {
namespace ledger {
namespace detail {

struct WorkQueueSort
{
  bool operator()(WorkPtr const &a, WorkPtr const &b) const
  {
    return a->score() < b->score();
  }
};

}  // namespace detail

using WorkQueue = std::priority_queue<WorkPtr, std::vector<WorkPtr>, detail::WorkQueueSort>;

}  // namespace ledger
}  // namespace fetch