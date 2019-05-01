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

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Sin(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::Sin(x, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Cos(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::Cos(x, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> Tan(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::Tan(x, x);
  return x;
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ASin(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::ASin(x, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ACos(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::ACos(x, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATan(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::ATan(x, x);
  return x;
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> SinH(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::SinH(x, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> CosH(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::CosH(x, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> TanH(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::TanH(x, x);
  return x;
}

/**
 * method for taking the sine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ASinH(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::ASinH(x, x);
  return x;
}

/**
 * method for taking the cosine of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ACosH(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::ACosH(x, x);
  return x;
}

/**
 * method for taking the tangent of a value
 */
template <typename T>
fetch::math::meta::IfIsMath<T, T> ATanH(fetch::vm::VM *, T const &a)
{
  T x = T(a);
  fetch::math::ATanH(x, x);
  return x;
}

static void CreateTrigonometry(std::shared_ptr<fetch::vm::Module> module)
{
  module->CreateFreeFunction<float_t>("sin", &Sin<float_t>);
  module->CreateFreeFunction<double_t>("sin", &Sin<double_t>);
  module->CreateFreeFunction<float_t>("cos", &Cos<float_t>);
  module->CreateFreeFunction<double_t>("cos", &Cos<double_t>);
  module->CreateFreeFunction<float_t>("tan", &Tan<float_t>);
  module->CreateFreeFunction<double_t>("tan", &Tan<double_t>);

  module->CreateFreeFunction<float_t>("asin", &ASin<float_t>);
  module->CreateFreeFunction<double_t>("asin", &ASin<double_t>);
  module->CreateFreeFunction<float_t>("acos", &ACos<float_t>);
  module->CreateFreeFunction<double_t>("acos", &ACos<double_t>);
  module->CreateFreeFunction<float_t>("atan", &ATan<float_t>);
  module->CreateFreeFunction<double_t>("atan", &ATan<double_t>);

  module->CreateFreeFunction<float_t>("sinh", &SinH<float_t>);
  module->CreateFreeFunction<double_t>("sinh", &SinH<double_t>);
  module->CreateFreeFunction<float_t>("cosh", &CosH<float_t>);
  module->CreateFreeFunction<double_t>("cosh", &CosH<double_t>);
  module->CreateFreeFunction<float_t>("tanh", &TanH<float_t>);
  module->CreateFreeFunction<double_t>("tanh", &TanH<double_t>);

  module->CreateFreeFunction<float_t>("asinh", &ASinH<float_t>);
  module->CreateFreeFunction<double_t>("asinh", &ASinH<double_t>);
  module->CreateFreeFunction<float_t>("acosh", &ACosH<float_t>);
  module->CreateFreeFunction<double_t>("acosh", &ACosH<double_t>);
  module->CreateFreeFunction<float_t>("atanh", &ATanH<float_t>);
  module->CreateFreeFunction<double_t>("atanh", &ATanH<double_t>);
}

}  // namespace vm_modules
}  // namespace fetch
