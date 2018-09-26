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

//#include "math/kernels/approx_exp.hpp"
//#include "math/kernels/approx_log.hpp"
//#include "math/kernels/approx_logistic.hpp"
//#include "math/kernels/approx_soft_max.hpp"
//#include "math/kernels/basic_arithmetics.hpp"
//#include "math/kernels/relu.hpp"
//#include "math/kernels/sign.hpp"
//#include "math/kernels/standard_functions.hpp"

#include "math/free_functions/free_functions.hpp"
#include "ml/variable.hpp"
//
//#include "core/assert.hpp"
//#include "core/meta/type_traits.hpp"
//#include "math/ndarray_broadcast.hpp"
//#include "vectorise/memory/range.hpp"
//#include <algorithm>
//#include <cassert>
//#include <numeric>
//#include <vector>

namespace fetch {
namespace ml {
namespace ops {
namespace derivatives {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

template <typename ARRAY_TYPE, typename T>
void MeanSquareError(ARRAY_TYPE error, ARRAY_TYPE &grads)
{
  grads = error;
}

template <typename ARRAY_TYPE>
void sigmoid(ARRAY_TYPE error, ARRAY_TYPE &grads)
{
  for (std::size_t i = 0; i < error.size(); ++i)
  {
    grads[i] = error[i] * (1 - error[i]);
  }
}

template <typename T>
void Relu(Variable<T> &cur_node)
{
  assert(cur_node.prev.size() == 1);

  auto &left = cur_node.prev[0];
  auto &dy   = cur_node.grad;

  for (std::size_t i = 0; i < left.data.size(); ++i)
  {
    if (left.data[i] > 0)
    {
      left.grad[i] += dy[i];
    }
    else
    {
      left.grad[i] = 0;
    }
  }
};

template <typename T>
void Sum(Variable<T> &cur_node)
{
  assert(cur_node.prev.size() == 1);
  cur_node.prev[0].grad += cur_node.grad;
};

template <typename T>
void Dot(Variable<T> &cur_node)
{
  assert(cur_node.prev.size() == 2);

  auto &left  = cur_node.prev[0];
  auto &right = cur_node.prev[1];
  auto &dy    = cur_node.grad;

  left.grad += fetch::math::DotTranspose(dy, right.data);
  right.grad += fetch::math::TransposeDot(left.data, dy);
};

};  // namespace derivatives
};  // namespace ops
};  // namespace ml
};  // namespace fetch
