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

#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm_modules/ledger/synergetic_job.hpp"
#include "vm/array.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

using namespace fetch::vm;

SynergeticJob::SynergeticJob(VM *vm, TypeId type_id)
  : Object{vm, type_id}
{}

void SynergeticJob::Bind(Module &module)
{
  module.CreateClassType<SynergeticJob>("SynergeticJob")
      .CreateMemberFunction("contractAddress", &SynergeticJob::contract_address)
      .CreateMemberFunction("id", &SynergeticJob::id)
      .CreateMemberFunction("epoch", &SynergeticJob::epoch)
      .CreateMemberFunction("problemCharge", &SynergeticJob::problem_charge)
      .CreateMemberFunction("workCharge", &SynergeticJob::work_charge)
      .CreateMemberFunction("clearCharge", &SynergeticJob::clear_charge)
      .CreateMemberFunction("totalCharge", &SynergeticJob::total_charge);

  module.GetClassInterface<IArray>().CreateInstantiationType<Array<Ptr<SynergeticJob>>>();
}

AddressPtr SynergeticJob::contract_address() const
{
  return contract_address_;
}

uint64_t SynergeticJob::id() const
{
  return id_;
}

uint64_t SynergeticJob::epoch() const
{
  return epoch_;
}

uint64_t SynergeticJob::problem_charge() const
{
  return problem_charge_;
}

uint64_t SynergeticJob::work_charge() const
{
  return work_charge_;
}

uint64_t SynergeticJob::clear_charge() const
{
  return clear_charge_;
}

uint64_t SynergeticJob::total_charge() const
{
  return problem_charge_ + work_charge_ + clear_charge_;
}

void SynergeticJob::set_id(uint64_t const &id)
{
  id_ = id;
}

void SynergeticJob::set_epoch(uint64_t const &epoch)
{
  epoch_ = epoch;
}

void SynergeticJob::set_problem_charge(uint64_t const &charge)
{
  problem_charge_ = charge;
}

void SynergeticJob::set_work_charge(uint64_t const &charge)
{
  work_charge_ = charge;
}

void SynergeticJob::set_clear_charge(uint64_t const &charge)
{
  clear_charge_ = charge;
}

void SynergeticJob::set_contract_address(AddressPtr address)
{
  contract_address_ = std::move(address);
}


}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
