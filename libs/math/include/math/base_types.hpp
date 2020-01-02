#pragma once
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
#include "math/tensor/tensor_declaration.hpp"

#include <cfenv>
#include <cstdint>
#include <limits>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace math {

using SizeType    = uint64_t;
using PtrDiffType = int64_t;
using SizeVector  = std::vector<SizeType>;
using SizeSet     = std::unordered_set<SizeType>;

constexpr SizeType NO_AXIS = SizeType(-1);

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, T> numeric_max()
{
  return std::numeric_limits<T>::max();
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, T> numeric_max()
{
  return T::FP_MAX;
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, T> numeric_min()
{
  return std::numeric_limits<T>::min();
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, T> numeric_min()
{
  return T::CONST_SMALLEST_FRACTION;
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, T> numeric_lowest()
{
  return std::numeric_limits<T>::lowest();
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, T> numeric_lowest()
{
  return T::FP_MIN;
}

template <typename T>
fetch::meta::IfIsFloat<T, T> static numeric_inf()
{
  return std::numeric_limits<T>::infinity();
}

template <typename T>
meta::IfIsFixedPoint<T, T> static numeric_inf()
{
  return T::POSITIVE_INFINITY;
}

template <typename T>
fetch::meta::IfIsFloat<T, T> static numeric_negative_inf()
{
  return -std::numeric_limits<T>::infinity();
}

template <typename T>
meta::IfIsFixedPoint<T, T> static numeric_negative_inf()
{
  return T::NEGATIVE_INFINITY;
}

template <typename T>
meta::IfIsFixedPoint<T, T> static function_tolerance()
{
  return T::TOLERANCE;
}

template <typename T>
fetch::meta::IfIsInteger<T, T> static function_tolerance()
{
  return T(0);
}

template <typename T>
fetch::meta::IfIsFloat<T, T> static function_tolerance()
{
  return T(1e-6);
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> is_nan(T const &val)
{
  return std::isnan(val);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> is_nan(T const &val)
{
  return T::IsNaN(val);
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> is_div_by_zero(T const &val)
{
  return std::isinf(val);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> is_div_by_zero(T const &val)
{
  auto tmp     = T::IsNegInfinity(val);
  auto tmp_pos = T::IsPosInfinity(val);
  auto ret     = (tmp || tmp_pos);
  return ret;
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> state_nan()
{
  return std::fetestexcept(FE_INVALID);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> state_nan()
{
  return T::IsStateNaN();
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> state_division_by_zero()
{
  return std::fetestexcept(FE_DIVBYZERO);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> state_division_by_zero()
{
  return T::IsStateDivisionByZero();
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> state_overflow()
{
  return std::fetestexcept(FE_OVERFLOW);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> state_overflow()
{
  return T::IsStateOverflow();
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> state_infinity()
{
  return std::fetestexcept(FE_DIVBYZERO);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> state_infinity()
{
  return T::IsStateInfinity();
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, void> state_clear()
{
  std::feclearexcept(FE_ALL_EXCEPT);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, void> state_clear()
{
  T::StateClear();
}

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, bool> is_inf(T const &val)
{
  return std::isinf(val);
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, bool> is_inf(T const &val)
{
  return (T::IsNegInfinity(val) || T::IsPosInfinity(val) || state_division_by_zero<T>());
}

template <typename T>
static constexpr meta::IfIsUnsignedInteger<T, T> Type(std::string const &val)
{
  if (std::stoll(val) < 0)
  {
    throw std::runtime_error("cannot initialise uint with negative value");
  }
  auto x = static_cast<T>(std::stoull(val));
  return x;
}

template <typename T>
static constexpr meta::IfIsSignedInteger<T, T> Type(std::string const &val)
{
  return T(std::stoll(val));
}

template <typename T>
static constexpr meta::IfIsFloat<T, T> Type(std::string const &val)
{
  return T(std::stod(val));
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, T> Type(std::string const &val)
{
  return T(val);
}

template <typename T, typename U>
static constexpr meta::IfIsFloat<T, T> AsType(U val, meta::IfIsInteger<U> * /*unused*/ = nullptr)
{
  return static_cast<T>(val);
}

template <typename T, typename U>
static constexpr meta::IfIsFixedPoint<T, T> AsType(U val,
                                                   meta::IfIsInteger<U> * /*unused*/ = nullptr)
{
  return static_cast<T>(val);
}

template <typename T, typename U>
static constexpr meta::IfIsFloat<T, T> AsType(U val, meta::IfIsFloat<U> * /*unused*/ = nullptr)
{
  return static_cast<T>(val);
}

}  // namespace math
}  // namespace fetch
