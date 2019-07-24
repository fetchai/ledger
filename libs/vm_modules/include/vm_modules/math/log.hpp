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
#include "math/standard_functions/log.hpp"
#include "vm/module.hpp"

#include <cmath>

namespace fetch {
namespace vm_modules {
namespace math {

template <typename T>
fetch::math::meta::IfIsMath<T, T> Log(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Log(a, x);
  return x;
}

template <typename T>
fetch::math::meta::IfIsMath<T, T> Log2(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Log2(a, x);
  return x;
}

template <typename T>
fetch::math::meta::IfIsMath<T, T> Log10(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Log10(a, x);
  return x;
}

inline void BindLog(fetch::vm::Module &module)
{
  module.CreateFreeFunction("log", &Log<float_t>);
  module.CreateFreeFunction("log", &Log<double_t>);
  module.CreateFreeFunction("log", &Log<fixed_point::fp32_t>);
  module.CreateFreeFunction("log", &Log<fixed_point::fp64_t>);

  module.CreateFreeFunction("log2", &Log2<float_t>);
  module.CreateFreeFunction("log2", &Log2<double_t>);
  module.CreateFreeFunction("log2", &Log2<fixed_point::fp32_t>);
  module.CreateFreeFunction("log2", &Log2<fixed_point::fp64_t>);

  module.CreateFreeFunction("log10", &Log10<float_t>);
  module.CreateFreeFunction("log10", &Log10<double_t>);
  module.CreateFreeFunction("log10", &Log10<fixed_point::fp32_t>);
  module.CreateFreeFunction("log10", &Log10<fixed_point::fp64_t>);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
