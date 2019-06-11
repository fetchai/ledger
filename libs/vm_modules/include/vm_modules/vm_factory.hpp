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

#include "vm/module.hpp"

#include "vm_modules/core/print.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"

#include "vm_modules/crypto/sha256.hpp"

#include "vm_modules/math/abs.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/math/random.hpp"

#include "vm_modules/ml/cross_entropy.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/mean_square_error.hpp"
#include "vm_modules/ml/tensor.hpp"

namespace fetch {
namespace vm_modules {

/**
 * VMFactory provides the user with convenient management of the VM
 * and its associated bindings
 *
 */
class VMFactory
{
public:
  /**
   * Get a module, the VMFactory will add whatever bindings etc. are considered in the 'standard
   * library'
   *
   * @return: The module
   */
  static std::shared_ptr<fetch::vm::Module> GetModule()
  {
    auto module = std::make_shared<fetch::vm::Module>();

    // core modules
    CreatePrint(*module);
    CreateToString(*module);
    ByteArrayWrapper::Bind(*module);

    // math modules
    CreateAbs(*module);
    CreateRand(module);
    UInt256Wrapper::Bind(*module);    

    // Crypto
    SHA256Wrapper::Bind(*module);

    // ml modules - order is important!!
    ml::CreateTensor(*module);
    ml::CreateGraph(*module);
    ml::CreateCrossEntropy(*module);
    ml::CreateMeanSquareError(*module);

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
                                          fetch::vm::Executable &                   executable)
  {
    std::vector<std::string> errors;

    // generate the compiler from the module
    auto   compiler = std::make_shared<fetch::vm::Compiler>(module.get());
    vm::IR ir;

    // compile the source
    bool const compiled = compiler->Compile(source, "default", ir, errors);

    if (!compiled)
    {
      errors.push_back("Failed to compile.");
      return errors;
    }

    fetch::vm::VM vm(module.get());  // TODO(tfr): refactor such that IR is first made exectuable
    if (!vm.GenerateExecutable(ir, "default_ir", executable, errors))
    {
      return errors;
    }

#ifndef NDEBUG
    std::ostringstream all_errors;
    for (auto const &error : errors)
    {
      all_errors << error << std::endl;
    }

    if (errors.size() > 0)
    {
      FETCH_LOG_WARN("VM_FACTORY", "Found badly constructed SC. Debug:\n", all_errors.str());
    }
#endif

    return errors;
  }

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
