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

#include "vm/analyser.hpp"
#include "vm/common.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "ledger/chain/address.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/upow/synergetic_base_types.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fetch {

class BitVector;

namespace byte_array {
class ConstByteArray;
}  // namespace byte_array

namespace math {

class BigUnsigned;

}  // namespace math

namespace vm {

class Module;
class Compiler;
class IR;
struct Executable;
struct Variant;

}  // namespace vm

namespace ledger {

class StorageInterface;

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

  /// @name Actions to be taken on the synergetic contract
  /// @{
  Status DefineProblem(ProblemData const &problem_data);
  Status Work(math::BigUnsigned const &nonce, WorkScore &score);
  Status Complete(uint64_t block, BitVector const &shards);
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

  StorageInterface *storage_{nullptr};
  VariantPtr        problem_;
  VariantPtr        solution_;
};

inline Digest const &SynergeticContract::digest() const
{
  return digest_;
}

inline std::string const &SynergeticContract::work_function() const
{
  return work_function_;
}

inline std::string const &SynergeticContract::problem_function() const
{
  return problem_function_;
}

inline std::string const &SynergeticContract::objective_function() const
{
  return objective_function_;
}

inline std::string const &SynergeticContract::clear_function() const
{
  return clear_function_;
}

inline void SynergeticContract::Attach(StorageInterface &storage)
{
  storage_ = &storage;
}

inline void SynergeticContract::Detach()
{
  storage_ = nullptr;
  problem_.reset();
  solution_.reset();
}

inline constexpr char const *ToString(SynergeticContract::Status status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case SynergeticContract::Status::SUCCESS:
    text = "Success";
    break;
  case SynergeticContract::Status::VM_EXECUTION_ERROR:
    text = "VM Execution Error";
    break;
  case SynergeticContract::Status::NO_STATE_ACCESS:
    text = "No State Access";
    break;
  case SynergeticContract::Status::GENERAL_ERROR:
    text = "General Error";
    break;
  }

  return text;
}

using SynergeticContractPtr = std::shared_ptr<SynergeticContract>;

}  // namespace ledger
}  // namespace fetch
