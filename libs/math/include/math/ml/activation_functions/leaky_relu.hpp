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

#include "math/comparison.hpp"
#include "math/standard_functions/exp.hpp"

#include <cassert>

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
  assert(t.size() == ret.size());
  using DataType = typename ArrayType::Type;

  auto it  = t.cbegin();
  auto rit = ret.begin();
  while (it.is_valid())
  {
    *rit = fetch::math::Max(*it, typename ArrayType::Type(0));
    if (*it >= DataType(0))
    {
      // f(x)=x for x>=0
      *rit = *it;
    }
    else
    {
      // f(x)=a*x for x<0
      *rit = Multiply(a, *it);
    }
    ++it;
    ++rit;
  }
}

template <typename ArrayType>
ArrayType LeakyRelu(ArrayType const &t, typename ArrayType::Type const &a)
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
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;

  // Test if input is broadcastable by batch dimension
  assert(t.shape().size() == ret.shape().size());
  assert(a.shape().at(a.shape().size() - 1) == 1);

  SizeType t_batch_dimension   = t.shape().size() - 1;
  SizeType a_batch_dimension   = a.shape().size() - 1;
  SizeType ret_batch_dimension = ret.shape().size() - 1;
  SizeType batch_size          = t.shape().at(t_batch_dimension);

  for (SizeType i{0}; i < batch_size; i++)
  {
    // Slice along batch dimension
    auto t_slice   = t.Slice(i, t_batch_dimension);
    auto a_slice   = a.Slice(0, a_batch_dimension);
    auto ret_slice = ret.Slice(i, ret_batch_dimension);

    auto it  = t_slice.begin();
    auto rit = ret_slice.begin();
    auto ait = a_slice.begin();

    while (it.is_valid())
    {
      *rit = fetch::math::Max(*it, typename ArrayType::Type(0));
      if (*it >= DataType(0))
      {
        // f(x)=x for x>=0
        *rit = *it;
      }
      else
      {
        // f(x)=a*x for x<0
        *rit = Multiply(*ait, *it);
      }
      ++it;
      ++rit;
      ++ait;
    }
  }
}

template <typename ArrayType>
ArrayType LeakyRelu(ArrayType const &t, ArrayType const &a)
{
  ArrayType ret(t.shape());
  LeakyRelu(t, a, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch
