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

#include "math/free_functions/free_functions.hpp"
#include "ml/variable.hpp"

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

template <typename VariablePtrType>
void MeanSquareError(VariablePtrType &cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];
  auto  temp3 = fetch::math::Subtract(left->data(), right->data());
  left->GradientAdd(temp3);  //
}

template <typename VariableType>
void CrossEntropyLoss(std::shared_ptr<VariableType> cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];

  auto n_data = static_cast<typename VariableType::ArrayType::Type>(left->data().shape()[0]);
  auto delta  = fetch::math::Subtract(left->data(), right->data());
  auto grad   = fetch::math::Divide(delta, n_data);
  left->GradientAdd(grad);

  //
  //  for (std::size_t i = 0; i < n_data; ++i)
  //  {
  //    // find the current correct index in the one-hot gt vector
  //    for (std::size_t j = 0; j < n_classes; ++j)
  //    {
  //      auto grad = left->data().At(i, j);
  //
  //      if (right->data().At(i, j) == 1.0)
  //      {
  //        grad -= 1;
  //      }
  ////      grad = grad / left->data().shape()[0];
  //      left->GradientValueAdd(i, j, grad);
  //    }
  //  }
}

template <typename VariablePtrType>
void SoftmaxCrossEntropyLoss(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];

  auto delta = fetch::math::Subtract(left->prev[0]->data(), right->data());
  left->GradientAdd(delta);
}

}  // namespace derivatives
}  // namespace ops
}  // namespace ml
}  // namespace fetch
