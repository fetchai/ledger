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
//  right->GradientAdd(fetch::math::ReduceSum(dy, 0));
    right->GradientAdd(fetch::math::ReduceMean(dy, 0));
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
void ReduceSum(VariablePtrType cur_node)
{
  assert(cur_node->prev.size() == 2);

  auto &left = cur_node->prev[0];
  auto &dy   = cur_node->grad();

  left->GradientAdd(dy);
  //  cur_node.prev[0].grad() += cur_node.grad();
}

};  // namespace derivatives
};  // namespace ops
};  // namespace ml
};  // namespace fetch
