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

#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"

namespace fetch {
namespace ml {
namespace details {
enum class ActivationType
{
  NOTHING,
  RELU,
  LEAKY_RELU,
  SIGMOID,
  LOG_SIGMOID,
  SOFTMAX,
  LOG_SOFTMAX
};

template <class T>
std::string CreateActivationLayer(ActivationType type, Graph<T> *g, std::string name,
                                  std::string input)
{
  switch (type)
  {
  case ActivationType::RELU:
    return g->template AddNode<fetch::ml::ops::Relu<T>>(name, {input});

  case ActivationType::LEAKY_RELU:
    return g->template AddNode<fetch::ml::ops::LeakyRelu<T>>(name, {input});

  case ActivationType::SIGMOID:
    return g->template AddNode<fetch::ml::ops::Sigmoid<T>>(name, {input});

  case ActivationType::LOG_SIGMOID:
    return g->template AddNode<fetch::ml::ops::LogSigmoid<T>>(name, {input});

  case ActivationType::SOFTMAX:
    return g->template AddNode<fetch::ml::ops::Softmax<T>>(name, {input});

  case ActivationType::LOG_SOFTMAX:
    return g->template AddNode<fetch::ml::ops::LogSoftmax<T>>(name, {input});

  default:
    return input;
  }
}
}  // namespace details
}  // namespace ml
}  // namespace fetch