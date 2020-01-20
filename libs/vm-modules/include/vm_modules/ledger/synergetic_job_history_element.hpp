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

#include "vm_modules/ledger/synergetic_job.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

class SynergeticJobHistoryElement : public vm::Object
{
public:
  using SynergeticJobArray = vm::Ptr<vm::Array<vm::Ptr<SynergeticJob>>>;
  using SelectedJobArray   = vm::Ptr<vm::Array<uint64_t>>;

  SynergeticJobHistoryElement(vm::VM *vm, vm::TypeId type_id, SynergeticJobArray jobs,
                              SelectedJobArray selected_jobs);

  static void Bind(vm::Module &module);

  void set_expected_charge(int64_t const &charge);
  void set_actual_charge(int64_t const &charge);

  SynergeticJobArray jobs();
  SelectedJobArray   selected_jobs();
  int64_t            expected_charge();
  int64_t            actual_charge();

private:
  SynergeticJobArray jobs_;
  SelectedJobArray   selected_jobs_;
  int64_t            expected_charge_;
  int64_t            actual_charge_;
};

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
