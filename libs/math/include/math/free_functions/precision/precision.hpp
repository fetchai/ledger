#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "math/kernels/standard_functions.hpp"
#include <cassert>

namespace fetch {
namespace math {

template <typename ArrayType>
void Erf(ArrayType &x)
{
  kernels::stdlib::Erf<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * complementary error function of x
 * @param x
 */
template <typename ArrayType>
void Erfc(ArrayType &x)
{
  kernels::stdlib::Erfc<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * ceiling round
 * @param x
 */
template <typename ArrayType>
void Ceil(ArrayType &x)
{
  kernels::stdlib::Ceil<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * floor rounding
 * @param x
 */
template <typename ArrayType>
void Floor(ArrayType &x)
{
  kernels::stdlib::Floor<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round towards 0
 * @param x
 */
template <typename ArrayType>
void Trunc(ArrayType &x)
{
  kernels::stdlib::Trunc<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in int format
 * @param x
 */
template <typename ArrayType>
void Round(ArrayType &x)
{
  kernels::stdlib::Round<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format
 * @param x
 */
template <typename ArrayType>
void Lround(ArrayType &x)
{
  kernels::stdlib::Lround<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int in float format with long long return
 * @param x
 */
template <typename ArrayType>
void Llround(ArrayType &x)
{
  kernels::stdlib::Llround<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * round to nearest int
 * @param x
 */
template <typename ArrayType>
void Rint(ArrayType &x)
{
  kernels::stdlib::Rint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Lrint(ArrayType &x)
{
  kernels::stdlib::Lrint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 *
 * @param x
 */
template <typename ArrayType>
void Llrint(ArrayType &x)
{
  kernels::stdlib::Llrint<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

} // math
} // fetch