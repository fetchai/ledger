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
#include "vm_modules/math/abs.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/log.hpp"
#include "vm_modules/math/pow.hpp"
#include "vm_modules/math/random.hpp"
#include "vm_modules/math/sqrt.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/trigonometry.hpp"

namespace fetch {
namespace vm_modules {
namespace math {

inline void BindMath(fetch::vm::Module &module)
{
  // bind math functions
  BindAbs(module);
  BindExp(module);
  BindLog(module);
  BindPow(module);
  BindRand(module);
  BindSqrt(module);
  BindTrigonometry(module);

  // bind math classes
  VMTensor::Bind(module);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
