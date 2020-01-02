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
#include "math/trigonometry.hpp"
#include "vm/fixed.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/math/trigonometry.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {

namespace {

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Sin(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Sin(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> SinPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::Sin(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Cos(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Cos(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> CosPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::Cos(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Tan(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::Tan(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> TanPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::Tan(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ASin(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::ASin(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ASinPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::ASin(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ACos(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::ACos(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ACosPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::ACos(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATan(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::ATan(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ATanPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::ATan(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATan2(VM * /*vm*/, T const &a, T const &b)
{
  T x;
  fetch::math::ATan2(a, b, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ATan2Ptr(VM *vm, Ptr<T> const &a, Ptr<T> const &b)
{
  fixed_point::fp128_t x;
  fetch::math::ATan2(a->data_, b->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> SinH(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::SinH(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> SinHPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::SinH(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> CosH(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::CosH(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> CosHPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::CosH(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> TanH(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::TanH(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> TanHPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::TanH(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ASinH(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::ASinH(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ASinHPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::ASinH(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ACosH(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::ACosH(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ACosHPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::ACosH(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATanH(VM * /*vm*/, T const &a)
{
  T x;
  fetch::math::ATanH(a, x);
  return x;
}

template <typename T>
IfIsPtrFixed128<T, Ptr<T>> ATanHPtr(VM *vm, Ptr<T> const &a)
{
  fixed_point::fp128_t x;
  fetch::math::ATanH(a->data_, x);
  return Ptr<Fixed128>(new Fixed128(vm, x));
}

}  // namespace

void BindTrigonometry(Module &module, bool const /*enable_experimental*/)
{
  module.CreateFreeFunction("sin", &Sin<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("cos", &Cos<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("tan", &Tan<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("asin", &ASin<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("acos", &ACos<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("atan", &ATan<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("atan2", &ATan2<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("sinh", &SinH<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("cosh", &CosH<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("tanh", &TanH<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("asinh", &ASinH<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("acosh", &ACosH<fixed_point::fp32_t>, ChargeAmount{1});
  module.CreateFreeFunction("atanh", &ATanH<fixed_point::fp32_t>, ChargeAmount{1});

  module.CreateFreeFunction("sin", &Sin<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("cos", &Cos<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("tan", &Tan<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("asin", &ASin<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("acos", &ACos<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("atan", &ATan<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("atan2", &ATan2<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("sinh", &SinH<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("cosh", &CosH<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("tanh", &TanH<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("asinh", &ASinH<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("acosh", &ACosH<fixed_point::fp64_t>, ChargeAmount{2});
  module.CreateFreeFunction("atanh", &ATanH<fixed_point::fp64_t>, ChargeAmount{2});

  module.CreateFreeFunction("sin", &SinPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("cos", &CosPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("tan", &TanPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("asin", &ASinPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("acos", &ACosPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("atan", &ATanPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("atan2", &ATan2Ptr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("sinh", &SinHPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("cosh", &CosHPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("tanh", &TanHPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("asinh", &ASinHPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("acosh", &ACosHPtr<Fixed128>, ChargeAmount{8});
  module.CreateFreeFunction("atanh", &ATanHPtr<Fixed128>, ChargeAmount{8});
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
