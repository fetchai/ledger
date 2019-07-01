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
#include "vm/module.hpp"

#include <random>

namespace fetch {
namespace vm_modules {
namespace math {

template <typename T>
fetch::meta::IfIsInteger<T, T> Rand(fetch::vm::VM *vm, T const &a = T{0}, T const &b = T{100})
{
  if (a >= b)
  {
    vm->RuntimeError("Invalid argument: Rand(a, b) must satisfy a < b");
    return T{0};
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  return std::uniform_int_distribution<T>{a, b}(mt);
}

template <typename T>
fetch::meta::IfIsFloat<T, T> Rand(fetch::vm::VM *vm, T const &a = T{.0}, T const &b = T{1.0})
{
  if (a >= b)
  {
    vm->RuntimeError("Invalid argument: Rand(a, b) must satisfy a < b");
    return T{.0};
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  return std::uniform_real_distribution<T>{a, b}(mt);
}

inline void BindRand(fetch::vm::Module &module)
{
  module.CreateFreeFunction<int16_t>("Rand", &Rand<int16_t>);
  module.CreateFreeFunction<int32_t>("Rand", &Rand<int32_t>);
  module.CreateFreeFunction<int64_t>("Rand", &Rand<int64_t>);
  module.CreateFreeFunction<uint16_t>("Rand", &Rand<uint16_t>);
  module.CreateFreeFunction<uint32_t>("Rand", &Rand<uint32_t>);
  module.CreateFreeFunction<uint64_t>("Rand", &Rand<uint64_t>);
  module.CreateFreeFunction<float_t>("Rand", &Rand<float_t>);
  module.CreateFreeFunction<double_t>("Rand", &Rand<double_t>);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
