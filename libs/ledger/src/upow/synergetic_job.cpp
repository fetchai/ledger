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

namespace fetch {
namespace ledger {

namespace {
  constexpr char const *LOGGING_NAME = "SynergeticJob";
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

void SynergeticJob::set_contract_address(chain::Address address)
{
  contract_address_ = std::move(address);
}

chain::Address const &SynergeticJob::contract_address() const
{
  return contract_address_;
}

uint64_t const &SynergeticJob::id() const
{
  return id_;
}

uint64_t const &SynergeticJob::epoch() const
{
  return epoch_;
}

uint64_t const &SynergeticJob::problem_charge() const
{
  return problem_charge_;
}

uint64_t const &SynergeticJob::work_charge() const
{
  return work_charge_;
}

uint64_t const &SynergeticJob::clear_charge() const
{
  return clear_charge_;
}

uint64_t SynergeticJob::total_charge() const
{
  return problem_charge_ + work_charge_ + clear_charge_;
}

SynergeticJob::VmType SynergeticJob::ToVMType(fetch::vm::VM *vm)
{
  VmType data{};

  try
  {
    data = vm->CreateNewObject<vm_modules::ledger::SynergeticJob>();
    data->set_contract_address(vm->CreateNewObject<vm::Address>(contract_address_));
    data->set_id(id_);
    data->set_epoch(epoch_);
    data->set_problem_charge(problem_charge_);
    data->set_work_charge(work_charge_);
    data->set_clear_charge(clear_charge_);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input jobs data: ", ex.what());
  }

  return data;
}

}  // namespace ledger
}  // namespace fetch
