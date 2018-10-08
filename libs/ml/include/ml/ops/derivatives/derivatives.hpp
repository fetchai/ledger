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
void AddBroadcast(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];
  auto &dy    = cur_node->grad();

  left->GradientAdd(dy);
  right->GradientAdd(fetch::math::ReduceSum(dy, 0));
  //  right->GradientAdd(fetch::math::ReduceMean(dy, 0));
}

template <typename VariablePtrType>
void Dot(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];
  auto &dy    = cur_node->grad();

  left->GradientAdd(fetch::math::DotTranspose(dy, right->data()));
  right->GradientAdd(fetch::math::TransposeDot(left->data(), dy));
}

template <typename VariablePtrType>
void Relu(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];
  auto &dy    = cur_node->grad();

  for (std::size_t i = 0; i < left->data().size(); ++i)
  {
    if (left->data()[i] > right->data()[i])
    {
      left->GradientValueAdd(i, dy[i]);
      //      left->GradientValueAdd(i, 1.0);
    }
    else
    {
      left->GradientSetZero(i);
    }
  }
}

template <typename VariablePtrType>
void ReduceSum(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left = cur_node->prev[0];
  auto &dy   = cur_node->grad();

  left->GradientAdd(dy);
  //  cur_node.prev[0].grad() += cur_node.grad();
}

template <typename VariablePtrType>
void MeanSquareError(VariablePtrType &cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];
  auto  temp3 = fetch::math::Subtract(left->data(), right->data());

  // minibatch gradient descent
  double n     = temp3.shape()[0];
  auto   temp4 = fetch::math::ReduceMean(temp3, 0);

  for (std::size_t idx = 0; idx < n; ++idx)
  {
    for (std::size_t i = 0; i < temp3.shape()[1]; ++i)
    {
      temp3.Set(idx, i, temp4.At(i));
      //      temp3.Set(idx, i, fetch::math::Divide(temp5.At(i), n));
    }
  }

  left->GradientAdd(temp3);  //
}

template <typename VariablePtrType>
void CrossEntropyLoss(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];

  auto temp3 = fetch::math::Multiply(-1.0, right->data());
  auto temp4 = fetch::math::Divide(right->data(), left->data());

  left->GradientAdd(temp4);
}

template <typename VariablePtrType>
void Sigmoid(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 1);

  auto &left = cur_node->prev[0];
  //  auto  temp1 = fetch::math::Subtract(1.0, left->data());
  //  auto  temp2 = fetch::math::Multiply(left->data(), temp1);
  //  left->GradientAdd(temp2);

  auto temp1 = fetch::math::Subtract(1.0, cur_node->data());
  auto temp2 = fetch::math::Multiply(cur_node->data(), temp1);
  left->GradientAdd(temp2);
}

};  // namespace derivatives
};  // namespace ops
};  // namespace ml
};  // namespace fetch
