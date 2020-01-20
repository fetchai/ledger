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

#include "ledger/upow/synergetic_job.hpp"
#include "vm/array.hpp"
#include "vm/vm.hpp"
#include "vm_modules/ledger/synergetic_job.hpp"
#include "vm_modules/ledger/synergetic_job_history_element.hpp"
#include <vector>

namespace fetch {
namespace ledger {

class SynergeticJobHistory
{
public:
  using VmSynergeticJob        = vm::Ptr<vm_modules::ledger::SynergeticJob>;
  using VmSynergeticJobArray   = vm::Ptr<vm::Array<VmSynergeticJob>>;
  using VmSelectedJobs         = vm::Ptr<vm::Array<uint64_t>>;
  using VmHistoryElementType   = vm_modules::ledger::SynergeticJobHistoryElement;
  using VmHistoryElement       = vm::Ptr<VmHistoryElementType>;
  using VmType                 = vm::Ptr<vm::Array<VmHistoryElement>>;

  SynergeticJobHistory(uint64_t const &cache_size);
  virtual ~SynergeticJobHistory() = default;

  void                  AddElement(vm::VM *vm, VmSynergeticJobArray &jobs, VmSelectedJobs &selected_jobs);
  VmHistoryElementType *back();
  std::size_t           size();
  VmType                Get(vm::VM *vm);

private:
  using UnderlyingArrayElement = vm::Array<VmHistoryElement >::ElementType;
  using UnderlyingArray        = std::vector<UnderlyingArrayElement>;

  uint64_t max_size_;
  UnderlyingArray history_;

};

}  // namespace ledger
}  // namespace fetch
