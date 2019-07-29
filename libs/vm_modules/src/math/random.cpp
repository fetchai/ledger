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
#include "vm_modules/math/random.hpp"

#include <random>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

template <typename T>
fetch::meta::IfIsInteger<T, T> Rand(VM *vm, T const &a = T{0}, T const &b = T{100})
{
  if (a >= b)
  {
    vm->RuntimeError("Invalid argument: rand(a, b) must satisfy a < b");
    return T{0};
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  return std::uniform_int_distribution<T>{a, b}(mt);
}

template <typename T>
fetch::meta::IfIsFloat<T, T> Rand(VM *vm, T const &a = T{.0}, T const &b = T{1.0})
{
  if (a >= b)
  {
    vm->RuntimeError("Invalid argument: rand(a, b) must satisfy a < b");
    return T{.0};
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  return std::uniform_real_distribution<T>{a, b}(mt);
}

}  // namespace

void BindRand(Module &module)
{
  module.CreateFreeFunction("rand", &Rand<int16_t>);
  module.CreateFreeFunction("rand", &Rand<int32_t>);
  module.CreateFreeFunction("rand", &Rand<int64_t>);
  module.CreateFreeFunction("rand", &Rand<uint16_t>);
  module.CreateFreeFunction("rand", &Rand<uint32_t>);
  module.CreateFreeFunction("rand", &Rand<uint64_t>);
  module.CreateFreeFunction("rand", &Rand<float_t>);
  module.CreateFreeFunction("rand", &Rand<double_t>);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
