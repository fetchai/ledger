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

#include "ledger/chain/address.hpp"
#include "ledger/upow/work.hpp"

#include <unordered_map>
#include <vector>

namespace fetch {
namespace ledger {

class Solutions
{
public:
  // Construction / Destruction
  Solutions() = default;
  Solutions(Solutions const &) = delete;
  Solutions(Solutions &&) = delete;
  ~Solutions() = default;

  void Update(WorkPtr const &work);
  WorkPtr Remove(Address const &address);
  void Clear();

  // Operators
  Solutions &operator=(Solutions const &) = delete;
  Solutions &operator=(Solutions &&) = delete;

private:
  using UnderlyingMap = std::unordered_map<Address, WorkPtr>;

  UnderlyingMap work_map_;
};

inline void Solutions::Update(WorkPtr const &work)
{
  bool update_solution{true};

  // determine if an existing solution is already present
  auto existing = work_map_.find(work->contract_digest());
  if (existing != work_map_.end())
  {
    if (existing->second->score() >= work->score())
    {
      update_solution = false;
    }
  }

  // if required to do so update the solution
  if (update_solution)
  {
    work_map_[work->contract_digest()] = work;
  }
}

inline WorkPtr Solutions::Remove(Address const &address)
{
  WorkPtr solution = work_map_[address];
  work_map_.erase(address);
  return solution;
}

inline void Solutions::Clear()
{
  work_map_.clear();
}

}  // namespace ledger
}  // namespace fetch