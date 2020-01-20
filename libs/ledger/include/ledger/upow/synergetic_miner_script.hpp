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

#include "chain/address.hpp"
#include "ledger/fees/chargeable.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "ledger/upow/synergetic_job.hpp"
#include "ledger/upow/synergetic_job_history.hpp"
#include "variant/variant.hpp"
#include "vm/analyser.hpp"
#include "vm/common.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/ledger/random.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fetch {

class BitVector;

namespace byte_array {
class ConstByteArray;
}  // namespace byte_array

namespace vectorise {
template <uint16_t S>
class UInt;
}

namespace vm {

class Module;
class Compiler;
class IR;
struct Executable;
struct Variant;

}  // namespace vm

namespace ledger {

class SynergeticMinerScript
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using SynergeticJobs = std::vector<std::unique_ptr<SynergeticJob>>;
  using JobList        = std::vector<uint64_t>;

  enum class Status
  {
    SUCCESS = 0,
    VM_EXECUTION_ERROR,
    GENERAL_ERROR
  };

  explicit SynergeticMinerScript(ConstByteArray const &source);
  ~SynergeticMinerScript() = default;

  // Accessors
  std::string const &mine_jobs_function() const;

  // Setters
  void set_balance(uint64_t const &balance);
  void set_back_expected_charge(int64_t const &charge);

  /// @name Actions to be taken on the synergetic contract
  /// @{
  Status GenerateJobList(SynergeticJobs const &jobs, JobList &generated_job_list, uint64_t balance);
  /// @}

private:
  using ModulePtr       = std::shared_ptr<vm::Module>;
  using CompilerPtr     = std::shared_ptr<vm::Compiler>;
  using IRPtr           = std::shared_ptr<vm::IR>;
  using ExecutablePtr   = std::shared_ptr<vm::Executable>;
  using VariantPtr      = std::shared_ptr<vm::Variant>;
  using VmRandomUniform = vm::Ptr<vm_modules::ledger::RandomUniform>;

  ModulePtr               module_;
  CompilerPtr             compiler_;
  IRPtr                   ir_;
  ExecutablePtr           executable_;
  std::unique_ptr<vm::VM> vm_;

  SynergeticJobHistory history_;

  std::string mine_jobs_function_;

  uint64_t current_balance_{0};

  VmRandomUniform random_unfiform_;

};

}  // namespace ledger
}  // namespace fetch
