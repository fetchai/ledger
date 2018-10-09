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
#include "ml/ops/derivatives/loss_functions.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

/*
 * Op for mean squared error
 */
template <typename VariablePtrType>
void MSEImplementation(VariablePtrType cur_node)
{
  cur_node->data() =
      fetch::math::MeanSquareError(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType MeanSquareError(VariablePtrType left, VariablePtrType right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::MeanSquareError(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    MSEImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> new_shape{left->shape()[0], 1};
  VariablePtrType          ret = sess.Variable(new_shape, "MSE", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/*
 * Cross entropy loss Op
 */
template <typename VariablePtrType>
void CELImplementation(VariablePtrType cur_node)
{
  cur_node->data() =
      fetch::math::CrossEntropyLoss(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariablePtrType, typename SessionType>
VariablePtrType CrossEntropyLoss(VariablePtrType left, VariablePtrType right, SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::CrossEntropyLoss(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    CELImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> new_shape = left->shape();
  VariablePtrType          ret       = sess.Variable(new_shape, "CEL", f_fn, b_fn, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

};  // namespace ops
};  // namespace ml
};  // namespace fetch
