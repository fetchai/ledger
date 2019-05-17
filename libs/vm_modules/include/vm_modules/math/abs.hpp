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

#include "math/meta/math_type_traits.hpp"
#include "math/standard_functions/abs.hpp"

namespace fetch {
namespace vm_modules {

/**
 * method for taking the absolute of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Abs(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::Abs(x, x);
  return x;
}

static void CreateAbs(fetch::vm::Module &module)
{
  module.CreateFreeFunction<int32_t>("Abs", &Abs<int32_t>);
  module.CreateFreeFunction<float_t>("Abs", &Abs<float_t>);
  module.CreateFreeFunction<double_t>("Abs", &Abs<double_t>);
}

inline void CreateAbs(std::shared_ptr<vm::Module> module)
{
  CreateAbs(*module.get());
}

}  // namespace vm_modules
}  // namespace fetch
