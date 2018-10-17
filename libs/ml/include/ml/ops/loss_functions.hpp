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
template <typename VariableType>

void MSEImplementation(std::shared_ptr<VariableType> cur_node)
{
  cur_node->data() =
      fetch::math::MeanSquareError(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariableType, typename SessionType>
std::shared_ptr<VariableType> MeanSquareError(std::shared_ptr<VariableType> left, std::shared_ptr<VariableType> right, SessionType &sess)
{
  // define the derivative
  std::function<void(std::shared_ptr<VariableType>)> b_fn = [](std::shared_ptr<VariableType> cur_node) {
    fetch::ml::ops::derivatives::MeanSquareError(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(std::shared_ptr<VariableType>)> f_fn = [](std::shared_ptr<VariableType> cur_node) {
    MSEImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> new_shape{left->shape()[0], 1};
  std::shared_ptr<VariableType>          ret = sess.Variable(new_shape, "MSE", f_fn, b_fn, false, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/*
 * Cross entropy loss Op
 */
template <typename VariableType>
void CELImplementation(std::shared_ptr<VariableType> cur_node)
{
  cur_node->data() =
      fetch::math::CrossEntropyLoss(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariableType, typename SessionType>
std::shared_ptr<VariableType> CrossEntropyLoss(std::shared_ptr<VariableType> left, std::shared_ptr<VariableType> right, SessionType &sess)
{
  // define the derivative
  std::function<void(std::shared_ptr<VariableType>)> b_fn = [](std::shared_ptr<VariableType> cur_node) {
    fetch::ml::ops::derivatives::CrossEntropyLoss(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(std::shared_ptr<VariableType>)> f_fn = [](std::shared_ptr<VariableType> cur_node) {
    CELImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> new_shape = left->shape();
  std::shared_ptr<VariableType>          ret       = sess.Variable(new_shape, "CEL", f_fn, b_fn, false, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

/*
 * Cross entropy loss Op
 */
template <typename VariableType, typename VariablePtrType = std::shared_ptr<VariableType>>
void SoftmaxCELImplementation(VariablePtrType cur_node)
{
  cur_node->data() =
      fetch::math::CrossEntropyLoss(cur_node->prev[0]->data(), cur_node->prev[1]->data());
}
template <typename VariableType, typename SessionType, typename VariablePtrType = std::shared_ptr<VariableType>>
VariablePtrType SoftmaxCrossEntropyLoss(VariablePtrType left, VariablePtrType right,
                                        SessionType &sess)
{
  // define the derivative
  std::function<void(VariablePtrType)> b_fn = [](VariablePtrType cur_node) {
    fetch::ml::ops::derivatives::SoftmaxCrossEntropyLoss(cur_node);
  };

  // define the forward function (i.e. the dot)
  std::function<void(VariablePtrType)> f_fn = [](VariablePtrType cur_node) {
    SoftmaxCELImplementation(cur_node);
  };

  // define the return variable with the Dot computation
  std::vector<std::size_t> new_shape = left->shape();
  VariablePtrType          ret = sess.Variable(new_shape, "Softmax_CEL", f_fn, b_fn, false, false);

  ret->prev.push_back(left);
  ret->prev.push_back(right);

  return ret;
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
