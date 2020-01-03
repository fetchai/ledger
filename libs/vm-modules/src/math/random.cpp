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
#include "vectorise/fixed_point/type_traits.hpp"
#include "vm/fixed.hpp"
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
    return T{0};
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  return std::uniform_real_distribution<T>{a, b}(mt);
}

template <typename T>
fetch::math::meta::IfIsNotFixedPoint128<T, T> Rand(VM *vm, T const &a = T{.0}, T const &b = T{1.0})
{
  if (a >= b)
  {
    vm->RuntimeError("Invalid argument: rand(a, b) must satisfy a < b");
    return T{0};
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  auto a_dbl = static_cast<double>(a);
  auto b_dbl = static_cast<double>(b);
  return fetch::math::AsType<T>(std::uniform_real_distribution<double>{a_dbl, b_dbl}(mt));
}

// We cannot use default values for parameters a & b, as they would have to be of the form:
// a = Ptr<T>(new vm::Fixed128(vm, fixed_point::fp128_t::_0))
// however vm variable is not accessible in this context.
template <typename T>
IfIsPtrFixed128<T, Ptr<T>> Rand(VM *vm, Ptr<T> const &a, Ptr<T> const &b)
{
  if (a->data_ >= b->data_)
  {
    vm->RuntimeError("Invalid argument: rand(a, b) must satisfy a < b");
    return Ptr<Fixed128>(new Fixed128(vm, fixed_point::fp128_t{0}));
  }

  std::random_device rd;
  std::mt19937_64    mt(rd());

  auto a_dbl = static_cast<double>(a->data_);
  auto b_dbl = static_cast<double>(b->data_);
  auto x     = fetch::math::AsType<fixed_point::fp128_t>(
      std::uniform_real_distribution<double>{a_dbl, b_dbl}(mt));
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

}  // namespace

void BindRand(Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateFreeFunction("rand", &Rand<int16_t>, ChargeAmount{1});
    module.CreateFreeFunction("rand", &Rand<int32_t>, ChargeAmount{1});
    module.CreateFreeFunction("rand", &Rand<int64_t>, ChargeAmount{1});
    module.CreateFreeFunction("rand", &Rand<uint16_t>, ChargeAmount{1});
    module.CreateFreeFunction("rand", &Rand<uint32_t>, ChargeAmount{1});
    module.CreateFreeFunction("rand", &Rand<uint64_t>, ChargeAmount{1});
    module.CreateFreeFunction("rand", &Rand<fixed_point::fp32_t>, ChargeAmount{4});
    module.CreateFreeFunction("rand", &Rand<fixed_point::fp64_t>, ChargeAmount{6});
    module.CreateFreeFunction("rand", &Rand<vm::Fixed128>, ChargeAmount{12});
  }
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
