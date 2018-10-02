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

template <typename LayerType>
void Relu(LayerType &cur_node)
{
  assert(cur_node.prev.size() == 1);

  auto &left = cur_node.prev[0];
  auto &dy   = cur_node.grad();

  for (std::size_t i = 0; i < left.data().size(); ++i)
  {
    if (left.data()[i] > 0)
    {
      std::cout << "RELU: Gradient Value Add " << std::endl;
      left.GradientValueAdd(i, dy[i]);
    }
    else
    {
      std::cout << "RELU: Gradient Set Zero " << std::endl;
      left.GradientSetZero(i);
    }
  }
}

template <typename LayerType>
void Sum(LayerType &cur_node)
{
  assert(cur_node.prev.size() == 1);

  auto &left = cur_node.prev[0];
  auto &dy   = cur_node.grad();

  left.GradientAdd(dy);
  //  cur_node.prev[0].grad() += cur_node.grad();
}

template <typename LayerType>
void Dot(LayerType &cur_node)
{
  assert(cur_node.prev.size() == 2);

  auto &left  = cur_node.prev[0];
  auto &right = cur_node.prev[1];
  auto &dy    = cur_node.grad();

  left.GradientAdd(fetch::math::DotTranspose(dy, right.data()));
  right.GradientAdd(fetch::math::TransposeDot(left.data(), dy));
}

template <typename LayerType>
void MeanSquareError(LayerType &cur_node)
{
  assert(cur_node.prev.size() == 2);

  auto &left  = cur_node.prev[0];
  auto &right = cur_node.prev[1];

  left.GradientAdd(fetch::math::Subtract(left.data(), right.data()));
}

template <typename LayerType>
void CrossEntropyLoss(LayerType &cur_node)
{
  std::cout << "CEL deriv prev size: " << cur_node.prev.size() << std::endl;
  std::cout << "CEL deriv prev size assign prevs" << std::endl;
  auto &left  = cur_node.prev[0];
  auto &right = cur_node.prev[1];

  std::cout << "CEL deriv Divide" << std::endl;
  std::cout << "right.data().shape[0]: " << right.data().shape()[0] << std::endl;
  std::cout << "right.data().shape[1]: " << right.data().shape()[1] << std::endl;
  std::cout << "left.data().shape[0]: " << left.data().shape()[0] << std::endl;
  std::cout << "left.data().shape[1]: " << left.data().shape()[1] << std::endl;
  typename LayerType::ArrayType temp = fetch::math::Divide(right.data(), left.data());

  std::cout << "temp size()[0]: " << temp.shape()[0] << std::endl;
  std::cout << "temp size()[1]: " << temp.shape()[1] << std::endl;

  std::cout << "CEL deriv Multiply" << fetch::math::Multiply(-1.0, temp).size() << std::endl;

  std::cout << "CEL GradientAdd" << std::endl;
  left.GradientAdd(fetch::math::Multiply(-1.0, fetch::math::Divide(right.data(), left.data())));
}

template <typename LayerType>
void Sigmoid(LayerType &cur_node)
{

  auto &left = cur_node.prev[0];

  left.GradientAdd(fetch::math::Multiply(left.data(), fetch::math::Subtract(1.0, left.data())));
}

};  // namespace derivatives
};  // namespace ops
};  // namespace ml
};  // namespace fetch
