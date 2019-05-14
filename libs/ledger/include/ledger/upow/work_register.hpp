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

#include "work.hpp"

#include <unordered_map>
#include <vector>

namespace fetch {
namespace consensus {

class WorkRegister
{
public:
  using ContractName = byte_array::ConstByteArray;

  void RegisterWork(Work const &work)
  {
    ContractName name = work.contract_name;
    if (work_pool_.find(name) == work_pool_.end())
    {
      work_pool_[name] = work;
      return;
    }

    auto cur_work = work_pool_[name];
    if (cur_work.score > work.score)
    {
      work_pool_[name] = work;
    }
  }

  Work ClearWorkPool(SynergeticContract contract)
  {
    ContractName name = contract->name;

    auto it = work_pool_.find(name);
    if (it == work_pool_.end())
    {
      return {};
    }

    Work ret = it->second;
    work_pool_.erase(it);
    return ret;
  }

private:
  std::unordered_map<ContractName, Work> work_pool_;
};

}  // namespace consensus
}  // namespace fetch