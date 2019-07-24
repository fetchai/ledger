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
#include "math/trigonometry.hpp"

namespace fetch {
namespace vm_modules {
namespace math {

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Sin(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Sin(a, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Cos(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Cos(a, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Tan(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::Tan(a, x);
  return x;
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ASin(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::ASin(a, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ACos(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::ACos(a, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATan(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::ATan(a, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATan2(fetch::vm::VM *, T const &a, T const &b)
{
  T x;
  fetch::math::ATan2(a, b, x);
  return x;
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> SinH(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::SinH(a, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> CosH(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::CosH(a, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> TanH(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::TanH(a, x);
  return x;
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ASinH(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::ASinH(a, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ACosH(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::ACosH(a, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATanH(fetch::vm::VM *, T const &a)
{
  T x;
  fetch::math::ATanH(a, x);
  return x;
}

inline void BindTrigonometry(fetch::vm::Module &module)
{
  module.CreateFreeFunction("sin", &Sin<float_t>);
  module.CreateFreeFunction("cos", &Cos<float_t>);
  module.CreateFreeFunction("tan", &Tan<float_t>);
  module.CreateFreeFunction("asin", &ASin<float_t>);
  module.CreateFreeFunction("acos", &ACos<float_t>);
  module.CreateFreeFunction("atan", &ATan<float_t>);
  module.CreateFreeFunction("atan2", &ATan2<float_t>);
  module.CreateFreeFunction("sinh", &SinH<float_t>);
  module.CreateFreeFunction("cosh", &CosH<float_t>);
  module.CreateFreeFunction("tanh", &TanH<float_t>);
  module.CreateFreeFunction("asinh", &ASinH<float_t>);
  module.CreateFreeFunction("acosh", &ACosH<float_t>);
  module.CreateFreeFunction("atanh", &ATanH<float_t>);

  module.CreateFreeFunction("sin", &Sin<double_t>);
  module.CreateFreeFunction("cos", &Cos<double_t>);
  module.CreateFreeFunction("tan", &Tan<double_t>);
  module.CreateFreeFunction("asin", &ASin<double_t>);
  module.CreateFreeFunction("acos", &ACos<double_t>);
  module.CreateFreeFunction("atan", &ATan<double_t>);
  module.CreateFreeFunction("atan2", &ATan2<double_t>);
  module.CreateFreeFunction("sinh", &SinH<double_t>);
  module.CreateFreeFunction("cosh", &CosH<double_t>);
  module.CreateFreeFunction("tanh", &TanH<double_t>);
  module.CreateFreeFunction("asinh", &ASinH<double_t>);
  module.CreateFreeFunction("acosh", &ACosH<double_t>);
  module.CreateFreeFunction("atanh", &ATanH<double_t>);

  module.CreateFreeFunction("sin", &Sin<fixed_point::fp32_t>);
  module.CreateFreeFunction("cos", &Cos<fixed_point::fp32_t>);
  module.CreateFreeFunction("tan", &Tan<fixed_point::fp32_t>);
  module.CreateFreeFunction("asin", &ASin<fixed_point::fp32_t>);
  module.CreateFreeFunction("acos", &ACos<fixed_point::fp32_t>);
  module.CreateFreeFunction("atan", &ATan<fixed_point::fp32_t>);
  module.CreateFreeFunction("atan2", &ATan2<fixed_point::fp32_t>);
  module.CreateFreeFunction("sinh", &SinH<fixed_point::fp32_t>);
  module.CreateFreeFunction("cosh", &CosH<fixed_point::fp32_t>);
  module.CreateFreeFunction("tanh", &TanH<fixed_point::fp32_t>);
  module.CreateFreeFunction("asinh", &ASinH<fixed_point::fp32_t>);
  module.CreateFreeFunction("acosh", &ACosH<fixed_point::fp32_t>);
  module.CreateFreeFunction("atanh", &ATanH<fixed_point::fp32_t>);

  module.CreateFreeFunction("sin", &Sin<fixed_point::fp64_t>);
  module.CreateFreeFunction("cos", &Cos<fixed_point::fp64_t>);
  module.CreateFreeFunction("tan", &Tan<fixed_point::fp64_t>);
  module.CreateFreeFunction("asin", &ASin<fixed_point::fp64_t>);
  module.CreateFreeFunction("acos", &ACos<fixed_point::fp64_t>);
  module.CreateFreeFunction("atan", &ATan<fixed_point::fp64_t>);
  module.CreateFreeFunction("atan2", &ATan2<fixed_point::fp64_t>);
  module.CreateFreeFunction("sinh", &SinH<fixed_point::fp64_t>);
  module.CreateFreeFunction("cosh", &CosH<fixed_point::fp64_t>);
  module.CreateFreeFunction("tanh", &TanH<fixed_point::fp64_t>);
  module.CreateFreeFunction("asinh", &ASinH<fixed_point::fp64_t>);
  module.CreateFreeFunction("acosh", &ACosH<fixed_point::fp64_t>);
  module.CreateFreeFunction("atanh", &ATanH<fixed_point::fp64_t>);
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
