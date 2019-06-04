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

#include "ledger/upow/synergetic_base_types.hpp"
#include "ledger/chain/address.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fetch {


namespace byte_array {
class ConstByteArray;
} // namespace byte_array

namespace math {

class BigUnsigned;

} // namespace math

namespace vm {

class Module;
class Compiler;
class IR;
struct Executable;
struct Variant;

} // namespace vm

namespace ledger {

class StorageInterface;

class SynergeticContract
{
public:

  using ConstByteArray = byte_array::ConstByteArray;

  enum class Status
  {
    SUCCESS = 0,
    VM_EXECUTION_ERROR,
    GENERAL_ERROR,
  };

  explicit SynergeticContract(ConstByteArray const &source);
  ~SynergeticContract() = default;

  // Accessors
  Address const &address() const;

  // Basic Contract Actions
  void Attach(StorageInterface &storage);
  void Detach();

  /// @name Actions to be taken on the synergetic contract
  /// @{
  Status DefineProblem();
  Status Clear();
  Status Work(math::BigUnsigned const &nonce, WorkScore &score);
  Status EvaluateObjective();
  Status GenerateSolution();
  /// @}

private:

  using ModulePtr = std::shared_ptr<vm::Module>;
  using CompilerPtr = std::shared_ptr<vm::Compiler>;
  using IRPtr = std::shared_ptr<vm::IR>;
  using ExecutablePtr = std::shared_ptr<vm::Executable>;
  using VariantPtr = std::shared_ptr<vm::Variant>;

  Address     address_;
  ModulePtr   module_;
  CompilerPtr compiler_;
  IRPtr       ir_;
  ExecutablePtr executable_;

  std::string problem_function_;
  std::string work_function_;
  std::string objective_function_;
  std::string clear_function_;
  std::string test_dag_generator_; /// EJF TODO
  int64_t clear_interval_{1};

  StorageInterface *storage_{nullptr};
  VariantPtr problem_;
};

inline Address const &SynergeticContract::address() const
{
  return address_;
}

inline void SynergeticContract::Attach(StorageInterface &storage)
{
  storage_ = &storage;
}

inline void SynergeticContract::Detach()
{
  storage_ = nullptr;
  problem_.reset();
}

inline char const *ToString(SynergeticContract::Status status)
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
  case SynergeticContract::Status::GENERAL_ERROR:
    return "General Error";
    break;
  }

  return text;
}

using SynergeticContractPtr = std::shared_ptr<SynergeticContract>;

#if 0

struct SynergeticContractClass
{
  using SynergeticContract = std::shared_ptr<SynergeticContractClass>;
  using UniqueCompiler     = std::unique_ptr<vm::Compiler>;

  static SynergeticContract New(UniqueCompiler & /*compiler*/, byte_array::ByteArray const &name,
                                std::string const &source, std::vector<std::string> &errors)
  {
    if (source.empty())
    {
      errors.push_back("No source present for synergetic contract.");
      return nullptr;
    }

    SynergeticContract ret = std::make_shared<SynergeticContractClass>();
    ret->name              = name;

    ret->work_function      = "";
    ret->objective_function = "";
    ret->problem_function   = "";
    ret->clear_function     = "";

    // fetch::vm::Strings err;
    // std::string        str_name = static_cast<std::string>(name);

    // Compiling contract
    // if (!compiler->Compile(source, str_name, ret->script, errors))
    //{
    //  return nullptr;
    //}

    // Finding work and objective functions
    // for (auto &f : ret->script.functions)
    //{
    //  bool is_work               = false;
    //  bool is_objective          = false;
    //  bool is_problem            = false;
    //  bool is_clear_function     = false;
    //  bool is_test_dag_generator = false;

    //  for (auto &a : f.annotations)
    //  {
    //    is_work |= (a.name == "@work");
    //    is_objective |= (a.name == "@objective");
    //    is_problem |= (a.name == "@problem");
    //    is_clear_function |= (a.name == "@clear");
    //    is_test_dag_generator |= (a.name == "@test_dag_generator");
    //  }

    //  if (is_work)
    //  {
    //    if (!ret->work_function.empty())
    //    {
    //      errors.push_back("Synergetic contract can only have one work function.");
    //      return nullptr;
    //    }
    //    ret->work_function = f.name;
    //  }

    //  if (is_objective)
    //  {
    //    if (!ret->objective_function.empty())
    //    {
    //      errors.push_back("Synergetic contract can only have one objective function.");
    //      return nullptr;
    //    }
    //    ret->objective_function = f.name;
    //  }

    //  if (is_problem)
    //  {
    //    if (!ret->problem_function.empty())
    //    {
    //      errors.push_back("Synergetic contract can only have one problem definition function.");
    //      return nullptr;
    //    }
    //    ret->problem_function = f.name;
    //  }

    //  if (is_clear_function)
    //  {
    //    if (!ret->clear_function.empty())
    //    {
    //      errors.push_back("Synergetic contract can only have one clear function.");
    //      return nullptr;
    //    }
    //    ret->clear_function = f.name;
    //  }

    //  if (is_test_dag_generator)
    //  {
    //    if (!ret->test_dag_generator.empty())
    //    {
    //      errors.push_back("Synergetic contract can only have one test generator function.");
    //      return nullptr;
    //    }
    //    ret->test_dag_generator = f.name;
    //  }
    //}

    if (ret->work_function.empty())
    {
      errors.push_back("Synergetic contract must have a work function");
      return nullptr;
    }

    if (ret->objective_function == "")
    {
      errors.push_back("Synergetic contract must have an objective function");
      return nullptr;
    }

    if (ret->problem_function == "")
    {
      errors.push_back("Synergetic contract must have an problem defition function");
      return nullptr;
    }

    if (ret->clear_function == "")
    {
      errors.push_back("Synergetic contract must have a contest clearing function");
      return nullptr;
    }

    // TODO: Test the function signatures of the contract

    return ret;
  }

  byte_array::ByteArray name;
  // vm::Script            script;

  std::string problem_function{""};
  std::string work_function{""};
  std::string objective_function{""};
  std::string clear_function{""};
  std::string test_dag_generator{""};

  int64_t clear_interval{1};
};

using SynergeticContract = SynergeticContractClass::SynergeticContract;
#endif

}  // namespace ledger
}  // namespace fetch

