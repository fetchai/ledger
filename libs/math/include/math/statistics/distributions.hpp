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

#include "math/kernels/standard_functions.hpp"
#include <cassert>

namespace fetch {
namespace math {

/**
 * Returns the gamma function of x.
 * If the magnitude of x is too large, an overflow range error occurs.
 * If too small, an underflow range error may occur.
 * If x is zero or a negative integer for which the function is asymptotic, it may cause a domain
 * error or a pole error (or none, depending on implementation).
 * @param x
 */
template <typename ArrayType>
void Tgamma(ArrayType &x)
{
  kernels::stdlib::Tgamma<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

/**
 * Returns the log of the gamma function of x.
 * If the magnitude of x is too large, an overflow range error occurs.
 * If too small, an underflow range error may occur.
 * If x is zero or a negative integer for which the function is asymptotic, it may cause a domain
 * error or a pole error (or none, depending on implementation).
 * @param x
 */
template <typename ArrayType>
void Lgamma(ArrayType &x)
{
  kernels::stdlib::Lgamma<typename ArrayType::Type> kernel;
  x.data().in_parallel().Apply(kernel, x.data());
}

}  // namespace math
}  // namespace fetch
