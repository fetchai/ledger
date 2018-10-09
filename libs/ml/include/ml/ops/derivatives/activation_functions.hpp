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
void Sigmoid(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 1);

  auto &left = cur_node->prev[0];
  auto &dy   = cur_node->grad();

  auto temp1 = fetch::math::Subtract(1.0, cur_node->data());
  auto temp2 = fetch::math::Multiply(cur_node->data(), temp1);
  auto temp3 = fetch::math::Multiply(dy, temp2);
  left->GradientAdd(temp3);
}

template <typename VariablePtrType>
void Relu(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left  = cur_node->prev[0];
  auto &right = cur_node->prev[1];
  auto &dy    = cur_node->grad();

  //  // identical???
  //  auto new_grads = fetch::math::Maximum(left->data(), right->data());
  //  auto new_grads2 = cur_node->data();
  //
  //  for (std::size_t idx = 0; idx < new_grads.size(); ++idx) {
  //    std::cout << "new_grads[" << idx << "]: " << new_grads[idx] << " : " << new_grads2[idx] <<
  //    std::endl;
  //  }
  //
  //
  //  left->GradientAdd(fetch::math::Dot(dy, cur_node->data()));
  //  //  left->GradientAdd(fetch::math::Multiply(dy, new_grads));

  for (std::size_t i = 0; i < left->data().size(); ++i)
  {
    if (left->data()[i] > right->data()[i])
    {
      left->GradientValueAdd(i, dy[i]);
    }
    else
    {
      //      left->GradientSetZero(i);
    }
  }
}

template <typename VariablePtrType>
void LeakyRelu(VariablePtrType cur_node)
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
    }
    else
    {
      left->GradientValueAdd(i, fetch::math::Multiply(0.01, dy[i]));
    }
  }
}
};  // namespace derivatives
};  // namespace ops
};  // namespace ml
};  // namespace fetch
