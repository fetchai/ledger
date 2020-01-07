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

#include "math/meta/math_type_traits.hpp"
#include "math/standard_functions/log.hpp"
#include "vm/fixed.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/math/log.hpp"

#include <cmath>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

template <typename T>
fetch::math::meta::IfIsMath<T, T> Log(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Log(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> LogPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x = a->data_;
  fetch::math::Log(x, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

template <typename T>
fetch::math::meta::IfIsMath<T, T> Log2(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Log2(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> Log2Ptr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x = a->data_;
  fetch::math::Log2(x, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

template <typename T>
fetch::math::meta::IfIsMath<T, T> Log10(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Log10(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> Log10Ptr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x = a->data_;
  fetch::math::Log10(x, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

}  // namespace

void BindLog(Module &module, bool const /*enable_experimental*/)
{
  // charge estimates based on benchmarking in math/benchmark
  module.CreateFreeFunction("log", &Log<fixed_point::fp32_t>, ChargeAmount{6});
  module.CreateFreeFunction("log", &Log<fixed_point::fp64_t>, ChargeAmount{8});
  module.CreateFreeFunction("log", &LogPtr<Fixed128>, ChargeAmount{12});

  module.CreateFreeFunction("log2", &Log2<fixed_point::fp32_t>, ChargeAmount{6});
  module.CreateFreeFunction("log2", &Log2<fixed_point::fp64_t>, ChargeAmount{8});
  module.CreateFreeFunction("log2", &Log2Ptr<Fixed128>, ChargeAmount{12});

  module.CreateFreeFunction("log10", &Log10<fixed_point::fp32_t>, ChargeAmount{6});
  module.CreateFreeFunction("log10", &Log10<fixed_point::fp64_t>, ChargeAmount{8});
  module.CreateFreeFunction("log10", &Log10Ptr<Fixed128>, ChargeAmount{12});
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
