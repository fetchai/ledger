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

#include "core/assert.hpp"
#include "math/comparison.hpp"
#include "math/standard_functions/exp.hpp"

namespace fetch {
namespace math {

/**
 * Leaky rectifier unit - Alpha value as scalar
 * @tparam ArrayType
 * @param t input tensor
 * @param a scalar alpha value
 * @param ret return tensor
 */
template <typename ArrayType>
void LeakyRelu(ArrayType const &t, typename ArrayType::Type const &a, ArrayType &ret)
{
  ASSERT(t.size() == ret.size());
  using DataType = typename ArrayType::Type;

  typename ArrayType::SizeType idx(0);
  for (auto const &val : t)
  {
    if (val >= DataType(0))
    {
      // f(x)=x for x>=0
      ret.Set(idx, val);
    }
    else
    {
      // f(x)=a*x for x<0
      Multiply(a, val, ret.At(idx));
    }
    ++idx;
  }
}

template <typename ArrayType>
ArrayType LeakyRelu(ArrayType const &t, typename ArrayType::Type &a)
{
  ArrayType ret(t.shape());
  LeakyRelu(t, a, ret);
  return ret;
}

/**
 * Leaky rectifier unit - Alpha values as vector
 * @tparam ArrayType
 * @param t input tensor
 * @param a vector of alpha values
 * @param ret return tensor
 */
template <typename ArrayType>
void LeakyRelu(ArrayType const &t, ArrayType const &a, ArrayType &ret)
{
  ASSERT(t.size() == ret.size());
  using DataType = typename ArrayType::Type;

  typename ArrayType::SizeType idx(0);
  for (auto const &val : t)
  {
    if (val >= DataType(0))
    {
      // f'(x)=x for x>=0
      ret.Set(idx, val);
    }
    else
    {
      // f'(x)=a*x for x<0
      Multiply(a.At(idx), val, ret.At(idx));
    }
    ++idx;
  }
}

template <typename ArrayType>
ArrayType LeakyRelu(ArrayType const &t, ArrayType &a)
{
  ArrayType ret(t.shape());
  LeakyRelu(t, a, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
