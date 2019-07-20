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
#include "math/fundamental_operators.hpp"
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
    if (*it >= static_cast<DataType>(0))
    {
      // f(x)=x for x>=0
      *rit = *it;
    }
    else
    {
      // f(x)=a*x for x<0
      Multiply(a, *it, *rit);
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

  // Test if input is broadcastable by batch dimension
  assert(t.shape().size() == ret.shape().size());
  assert(a.shape().at(a.shape().size() - 1) == 1);

  SizeType batch_size = t.shape().at(t.shape().size() - 1);

  for (SizeType i{0}; i < batch_size; i++)
  {
    // View along batch dimension
    auto t_view   = t.View(i);
    auto a_view   = a.View(0);
    auto ret_view = ret.View(i);

    auto it  = t_view.begin();
    auto rit = ret_view.begin();
    auto ait = a_view.begin();

    while (it.is_valid())
    {
      *rit = fetch::math::Max(*it, typename ArrayType::Type(0));
      if (*it >= static_cast<DataType>(0))
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
