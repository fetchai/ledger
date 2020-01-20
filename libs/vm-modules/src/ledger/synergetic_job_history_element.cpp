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
#include "vm_modules/ledger/synergetic_job_history_element.hpp"
#include "vm/array.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

using namespace fetch::vm;

SynergeticJobHistoryElement::SynergeticJobHistoryElement(VM *vm, TypeId type_id, SynergeticJobArray jobs,
    SelectedJobArray selected_jobs)
  : Object{vm, type_id}
  , jobs_{std::move(jobs)}
  , selected_jobs_{std::move(selected_jobs)}
{}

void SynergeticJobHistoryElement::Bind(Module &module)
{
  module.CreateClassType<SynergeticJobHistoryElement>("SynergeticJobHistoryElement")
      .CreateMemberFunction("jobs", &SynergeticJobHistoryElement::jobs)
      .CreateMemberFunction("selectedJobs", &SynergeticJobHistoryElement::selected_jobs)
      .CreateMemberFunction("expectedCharge", &SynergeticJobHistoryElement::expected_charge)
      .CreateMemberFunction("actualCharge", &SynergeticJobHistoryElement::actual_charge);

  module.GetClassInterface<IArray>().CreateInstantiationType<Array<Ptr<SynergeticJobHistoryElement>>>();
}

void SynergeticJobHistoryElement::set_actual_charge(int64_t const &charge)
{
  actual_charge_ = charge;
}

void SynergeticJobHistoryElement::set_expected_charge(int64_t const &charge)
{
  expected_charge_ = charge;
}

SynergeticJobHistoryElement::SynergeticJobArray SynergeticJobHistoryElement::jobs()
{
  return jobs_;
}

SynergeticJobHistoryElement::SelectedJobArray SynergeticJobHistoryElement::selected_jobs()
{
  return selected_jobs_;
}

int64_t SynergeticJobHistoryElement::expected_charge()
{
  return expected_charge_;
}

int64_t SynergeticJobHistoryElement::actual_charge()
{
  return actual_charge_;
}

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
