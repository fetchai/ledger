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

#include <memory>
#include <string>
#include <vector>

#include "vm_modules/core/panic.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/math/math.hpp"

#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "vm_modules/ml/ops/loss_functions/mean_square_error_loss.hpp"

#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/core/structured_data.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/crypto/sha256.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/sqrt.hpp"
#include "vm_modules/polyfill/bitshifting.hpp"
#include "vm_modules/polyfill/bitwise_ops.hpp"

namespace fetch {
namespace vm {

class Module;
struct Executable;
class VM;

}  // namespace vm

namespace vm_modules {

/**
 * VMFactory provides the user with convenient management of the VM
 * and its associated bindings
 *
 */
class VMFactory
{
public:
  using Errors    = std::vector<std::string>;
  using ModulePtr = std::shared_ptr<vm::Module>;

  enum Modules : uint64_t
  {
    MOD_CORE = (1ull << 0ull),
    MOD_MATH = (1ull << 1ull),
    MOD_SYN  = (1ull << 2ull),
    MOD_ML   = (1ull << 3ull),
  };

  enum UseCases : uint64_t
  {
    USE_SMART_CONTRACTS = (MOD_CORE | MOD_MATH | MOD_ML),
    USE_SYNERGETIC      = (MOD_CORE | MOD_MATH | MOD_SYN),
    USE_ALL             = (~uint64_t(0)),
  };

  /**
   * Get a module, the VMFactory will add whatever bindings etc. are considered in the 'standard
   * library'
   *
   * @return: The module
   */
  static std::shared_ptr<fetch::vm::Module> GetModule(uint64_t enabled)
  {
    auto module = std::make_shared<fetch::vm::Module>();

    // core modules
    if (MOD_CORE & enabled)
    {
      CreatePrint(*module);
      CreatePanic(*module);
      CreateToString(*module);
      CreateToBool(*module);

      StructuredData::Bind(*module);
    }

    // math modules
    if (MOD_MATH & enabled)
    {
      math::BindMath(*module);
    }

    // synergetic modules
    if (MOD_SYN & enabled)
    {
      ByteArrayWrapper::Bind(*module);
      math::UInt256Wrapper::Bind(*module);
      SHA256Wrapper::Bind(*module);

      math::BindExp(*module);
      math::BindSqrt(*module);
      BindBitShift(*module);
      BindBitwiseOps(*module);
    }

    // ml modules - order is important!!
    if (MOD_ML & enabled)
    {
      math::VMTensor::Bind(*module);
      ml::VMStateDict::Bind(*module);
      ml::VMGraph::Bind(*module);
      fetch::vm_modules::ml::VMCrossEntropyLoss::Bind(*module);
      ml::CreateMeanSquareErrorLoss(*module);
    }

    return module;
  }

  /**
   * Compile a source file, returning a executable
   *
   * @param: module The module which the user might have added various bindings/classes to etc.
   * @param: source The raw source to compile
   * @param: executable executable to fill
   *
   * @return: Vector of strings which represent errors found during compilation
   */
  static std::vector<std::string> Compile(std::shared_ptr<fetch::vm::Module> const &module,
                                          std::string const &                       source,
                                          fetch::vm::Executable &                   executable);
  /**
   * Get an instance of the VM after binding to a module
   *
   * @param: module Module which the user has added bindings to
   *
   * @return: An instance of the VM
   */
  static std::unique_ptr<fetch::vm::VM> GetVM(std::shared_ptr<fetch::vm::Module> const &module)
  {
    return std::make_unique<fetch::vm::VM>(module.get());
  }
};

}  // namespace vm_modules
}  // namespace fetch
