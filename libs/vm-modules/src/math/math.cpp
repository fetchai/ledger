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

#include "vm/module.hpp"
#include "vm_modules/math/abs.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/log.hpp"
#include "vm_modules/math/math.hpp"
#include "vm_modules/math/pow.hpp"
#include "vm_modules/math/random.hpp"
#include "vm_modules/math/read_csv.hpp"
#include "vm_modules/math/sqrt.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/trigonometry.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

void BindMath(Module &module, bool const enable_experimental)
{
  // bind math functions
  BindAbs(module, enable_experimental);
  BindExp(module, enable_experimental);
  BindLog(module, enable_experimental);
  BindPow(module, enable_experimental);
  BindRand(module, enable_experimental);
  BindSqrt(module, enable_experimental);
  BindTrigonometry(module, enable_experimental);

  // bind math classes
  VMTensor::Bind(module, enable_experimental);

  // ReadCSV depends on VMTensor so must be bound after it
  BindReadCSV(module, enable_experimental);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
