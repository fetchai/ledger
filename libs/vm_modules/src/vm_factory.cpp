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

#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/core/panic.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/structured_data.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/crypto/sha256.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/math.hpp"
#include "vm_modules/ml/ml.hpp"
#include "vm_modules/polyfill/bitshifting.hpp"
#include "vm_modules/polyfill/bitwise_ops.hpp"
#include "vm_modules/vm_factory.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

VMFactory::Errors VMFactory::Compile(std::shared_ptr<Module> const &module,
                                     std::string const &source, Executable &executable)
{
  std::vector<std::string> errors;

  // generate the compiler from the module
  auto compiler = std::make_shared<Compiler>(module.get());
  IR   ir;

  // compile the source
  bool const compiled = compiler->Compile(source, "default", ir, errors);

  if (!compiled)
  {
    errors.emplace_back("Failed to compile.");
    return errors;
  }

  VM vm(module.get());  // TODO(tfr): refactor such that IR is first made executable
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

std::shared_ptr<Module> VMFactory::GetModule(uint64_t enabled)
{
  auto module = std::make_shared<Module>();

  // core modules
  if (MOD_CORE & enabled)
  {
    CreatePrint(*module);
    CreatePanic(*module);
    CreateToString(*module);
    CreateToBool(*module);

    StructuredData::Bind(*module);
    ByteArrayWrapper::Bind(*module);
    math::UInt256Wrapper::Bind(*module);
    SHA256Wrapper::Bind(*module);
  }

  // math modules
  if (MOD_MATH & enabled)
  {
    math::BindMath(*module);
  }

  // synergetic modules
  if (MOD_SYN & enabled)
  {
    BindBitShift(*module);
    BindBitwiseOps(*module);
  }

  // ml modules
  if (MOD_ML & enabled)
  {
    ml::BindML(*module);
  }

  return module;
}

}  // namespace vm_modules
}  // namespace fetch
