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

#include <cstdint>
#include <unordered_set>
#include <vector>

#include "math/meta/math_type_traits.hpp"
#include "math/tensor_declaration.hpp"

namespace fetch {
namespace math {

using SizeType   = uint64_t;
using SizeVector = std::vector<SizeType>;
using SizeSet    = std::unordered_set<SizeType>;

constexpr SizeType NO_AXIS = SizeType(-1);

template <typename T>
static constexpr meta::IfIsNonFixedPointArithmetic<T, T> numeric_max()
{
  return std::numeric_limits<T>::max();
}

template <typename T>
static constexpr meta::IfIsFixedPoint<T, T> numeric_max()
{
  return T::Constants.MAX;
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
  return T::Constants.MIN;
}

template <typename T>
fetch::meta::IfIsFixedPoint<T, T> static function_tolerance()
{
  return T::TOLERANCE;
}

template <typename T>
fetch::meta::IfIsFloat<T, T> static function_tolerance()
{
  return T(1e-7);
}

}  // namespace math
}  // namespace fetch
