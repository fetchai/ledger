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
#include "vm_modules/vm_factory.hpp"

#include "vm_modules/core/print.hpp"
#include "vm_modules/core/type_convert.hpp"

#include "vm_modules/math/math.hpp"
#include "vm_modules/math/random.hpp"

#include "vm_modules/ml/cross_entropy.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/mean_square_error.hpp"
#include "vm_modules/ml/tensor.hpp"

#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/structured_data.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/crypto/sha256.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/sqrt.hpp"
#include "vm_modules/polyfill/bitshifting.hpp"
#include "vm_modules/polyfill/bitwise_ops.hpp"

namespace fetch {
namespace vm_modules {

/**
 * Get a module, the VMFactory will add whatever bindings etc. are considered in the 'standard
 * library'
 *
 * @return: The module
 */
std::shared_ptr<vm::Module> VMFactory::GetModule(uint64_t enabled)
{
  auto module = std::make_shared<fetch::vm::Module>();

  // core modules
  if (MOD_CORE & enabled)
  {
    CreatePrint(*module);
    CreateToString(*module);
    CreateToBool(*module);

    StructuredData::Bind(*module);
  }

  // math modules
  if (MOD_MATH & enabled)
  {
    CreateAbs(*module);
    /* CreateRand(module); */
  }

  // synergetic modules
  if (MOD_SYN & enabled)
  {
    ByteArrayWrapper::Bind(*module);
    BigNumberWrapper::Bind(*module);
    SHA256Wrapper::Bind(*module);

    BindExp(*module);
    BindSqrt(*module);
    BindBitShift(*module);
    BindBitwiseOps(*module);
  }

  // ml modules - order is important!!
  if (MOD_ML & enabled)
  {
    ml::CreateTensor(*module);
    ml::CreateGraph(*module);
    ml::CreateCrossEntropy(*module);
    ml::CreateMeanSquareError(*module);
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
VMFactory::Errors VMFactory::Compile(std::shared_ptr<fetch::vm::Module> const &module,
                                     std::string const &source, fetch::vm::Executable &executable)
{
  std::vector<std::string> errors;

  // generate the compiler from the module
  auto   compiler = std::make_shared<fetch::vm::Compiler>(module.get());
  vm::IR ir;

  // compile the source
  bool const compiled = compiler->Compile(source, "default", ir, errors);

  if (!compiled)
  {
    errors.emplace_back("Failed to compile.");
    return errors;
  }

  fetch::vm::VM vm(module.get());  // TODO(tfr): refactor such that IR is first made executable
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

  if (!errors.empty())
  {
    FETCH_LOG_WARN("VM_FACTORY", "Found badly constructed SC. Debug:\n", all_errors.str());
  }
#endif

  return errors;
}

}  // namespace vm_modules
}  // namespace fetch
