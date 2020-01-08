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

#include "math/standard_functions/sqrt.hpp"
#include "vm/fixed.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/math/sqrt.hpp"

#include <cmath>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

template <typename T>
fetch::math::meta::IfIsMath<T, T> Sqrt(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Sqrt(a, x);
  return x;
}

template <typename T>
fetch::meta::EnableIfSame<T, Ptr<Fixed128>, T> SqrtPtr(VM *vm, T const &a)
{
  fixed_point::fp128_t x = a->data_;
  fetch::math::Sqrt(x, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

}  // namespace

void BindSqrt(Module &module, bool const /*enable_experimental*/)
{
  module.CreateFreeFunction("sqrt", &Sqrt<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("sqrt", &Sqrt<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("sqrt", &SqrtPtr<Ptr<Fixed128>>, ChargeAmount{4});
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
