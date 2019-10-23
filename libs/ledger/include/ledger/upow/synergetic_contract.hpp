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

#include "chain/address.hpp"
#include "ledger/upow/synergetic_base_types.hpp"
#include "vm/analyser.hpp"
#include "vm/common.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

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

class StateAdapter;
class StorageInterface;
class TokenContract;

struct SynergeticContractContext
{
  SynergeticContractContext()                                  = default;
  SynergeticContractContext(SynergeticContractContext const &) = default;
  SynergeticContractContext(SynergeticContractContext &&)      = default;
  SynergeticContractContext &operator=(SynergeticContractContext const &) = default;
  SynergeticContractContext &operator=(SynergeticContractContext &&) = default;

  SynergeticContractContext(TokenContract *token_contract_param, chain::Address address,
                            StateAdapter *state_adapter_param)
    : token_contract{token_contract_param}
    , contract_address{std::move(address)}
    , state_adapter{state_adapter_param}
  {}  //???uninline

  TokenContract *token_contract{nullptr};
  chain::Address contract_address{};
  StateAdapter * state_adapter{nullptr};
};

class SynergeticContract
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using ProblemData    = std::vector<ConstByteArray>;

  enum class Status
  {
    SUCCESS = 0,
    VM_EXECUTION_ERROR,
    NO_STATE_ACCESS,
    GENERAL_ERROR,
  };

  explicit SynergeticContract(ConstByteArray const &source);
  ~SynergeticContract() = default;

  // Accessors
  Digest const &     digest() const;
  std::string const &work_function() const;
  std::string const &problem_function() const;
  std::string const &objective_function() const;
  std::string const &clear_function() const;

  // Basic Contract Actions
  void Attach(StorageInterface &storage);
  void Detach();

  void updateContractContext(SynergeticContractContext context)  //???uninline
  {
    context_ = std::move(context);
  }

  SynergeticContractContext const &context() const  //???uninline
  {
    return context_;
  }

  /// @name Actions to be taken on the synergetic contract
  /// @{
  Status DefineProblem(ProblemData const &problem_data);
  Status Work(vectorise::UInt<256> const &nonce, WorkScore &score);
  Status Complete(chain::Address const &address, BitVector const &shards);
  /// @}

  /// @name Synergetic State Access
  /// @{
  bool               HasProblem() const;
  vm::Variant const &GetProblem() const;
  bool               HasSolution() const;
  vm::Variant const &GetSolution() const;
  /// @}

private:
  using ModulePtr     = std::shared_ptr<vm::Module>;
  using CompilerPtr   = std::shared_ptr<vm::Compiler>;
  using IRPtr         = std::shared_ptr<vm::IR>;
  using ExecutablePtr = std::shared_ptr<vm::Executable>;
  using VariantPtr    = std::shared_ptr<vm::Variant>;

  Digest        digest_;
  ModulePtr     module_;
  CompilerPtr   compiler_;
  IRPtr         ir_;
  ExecutablePtr executable_;

  std::string problem_function_;
  std::string work_function_;
  std::string objective_function_;
  std::string clear_function_;

  StorageInterface *        storage_{nullptr};
  VariantPtr                problem_;
  VariantPtr                solution_;
  SynergeticContractContext context_{};
};

char const *ToString(SynergeticContract::Status status);

using SynergeticContractPtr = std::shared_ptr<SynergeticContract>;

}  // namespace ledger
}  // namespace fetch
