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

#include "ledger/upow/synergetic_job_history.hpp"

namespace fetch {
namespace ledger {

using VmSynergeticJob      = SynergeticJobHistory::VmSynergeticJob;
using VmSynergeticJobArray = SynergeticJobHistory::VmSynergeticJobArray;
using VmSelectedJobs       = SynergeticJobHistory::VmSelectedJobs;
using VmHistoryElement     = SynergeticJobHistory::VmHistoryElement;

namespace {
constexpr char const *LOGGING_NAME = "SynergeticJobHistory";

VmHistoryElement CreateHistoryElement(fetch::vm::VM *vm, VmSynergeticJobArray &jobs,
                                      VmSelectedJobs &selected_jobs)
{
  VmHistoryElement data{};
  try
  {
    data =
        vm->CreateNewObject<vm_modules::ledger::SynergeticJobHistoryElement>(jobs, selected_jobs);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "CreateHistoryElement: Failed to parse input jobs data: ", ex.what());
  }
  return data;
}

}  // namespace

SynergeticJobHistory::SynergeticJobHistory(uint64_t const &cache_size)
  : max_size_{cache_size}
{}

void SynergeticJobHistory::AddElement(vm::VM *vm, VmSynergeticJobArray &jobs,
                                      VmSelectedJobs &selected_jobs)
{
  auto data = CreateHistoryElement(vm, jobs, selected_jobs);

  if (data)
  {
    history_.emplace_back(std::move(data));
    while (history_.size() > max_size_)
    {
      history_.erase(history_.begin());
    }
  }
}

SynergeticJobHistory::VmHistoryElementType *SynergeticJobHistory::back()
{
  auto ptr = history_.back();
  return dynamic_cast<VmHistoryElementType *>(&(*ptr));
}

std::size_t SynergeticJobHistory::size()
{
  return history_.size();
}

SynergeticJobHistory::VmType SynergeticJobHistory::Get(vm::VM *vm)
{
  // create the array
  auto *ret = new vm::Array<VmHistoryElement>(vm, vm->GetTypeId<vm::IArray>(),
                                              vm->GetTypeId<VmHistoryElement>(),
                                              static_cast<int32_t>(history_.size()));

  // mov e the constructed elements over to the array
  ret->elements = std::move(history_);

  return VmType{ret};
}

}  // namespace ledger
}  // namespace fetch
