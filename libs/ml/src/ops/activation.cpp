//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/core/graph.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/gelu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"

namespace fetch {
namespace ml {
namespace details {

template <class T>
std::string AddActivationNode(ActivationType type, Graph<T> *g, std::string name, std::string input)
{
  switch (type)
  {
  case ActivationType::LEAKY_RELU:
    return g->template AddNode<fetch::ml::ops::LeakyRelu<T>>(name, {input});

  case ActivationType::LOG_SIGMOID:
    return g->template AddNode<fetch::ml::ops::LogSigmoid<T>>(name, {input});

  case ActivationType::LOG_SOFTMAX:
    return g->template AddNode<fetch::ml::ops::LogSoftmax<T>>(name, {input});

  case ActivationType::RELU:
    return g->template AddNode<fetch::ml::ops::Relu<T>>(name, {input});

  case ActivationType::SIGMOID:
    return g->template AddNode<fetch::ml::ops::Sigmoid<T>>(name, {input});

  case ActivationType::SOFTMAX:
    return g->template AddNode<fetch::ml::ops::Softmax<T>>(name, {input});

  case ActivationType::GELU:
    return g->template AddNode<fetch::ml::ops::Gelu<T>>(name, {input});

  default:
    return input;
  }
}

template <class T>
OperationsCount GetActivationCharge(ActivationType type)
{
  switch (type)
  {
  case ActivationType::LEAKY_RELU:
    return ops::LeakyRelu<T>::ChargeConstruct();

  case ActivationType::LOG_SIGMOID:
    return ops::LogSigmoid<T>::ChargeConstruct();

  case ActivationType::LOG_SOFTMAX:
    return ops::LogSoftmax<T>::ChargeConstruct();

  case ActivationType::RELU:
    return ops::Relu<T>::ChargeConstruct();

  case ActivationType::SIGMOID:
    return ops::Sigmoid<T>::ChargeConstruct();

  case ActivationType::SOFTMAX:
    return ops::Softmax<T>::ChargeConstruct();

  case ActivationType::GELU:
    return ops::Gelu<T>::ChargeConstruct();

  default:
    return 0;
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template std::string AddActivationNode<math::Tensor<int8_t>>(ActivationType               type,
                                                             Graph<math::Tensor<int8_t>> *g,
                                                             std::string name, std::string input);
template std::string AddActivationNode<math::Tensor<int16_t>>(ActivationType                type,
                                                              Graph<math::Tensor<int16_t>> *g,
                                                              std::string name, std::string input);
template std::string AddActivationNode<math::Tensor<int32_t>>(ActivationType                type,
                                                              Graph<math::Tensor<int32_t>> *g,
                                                              std::string name, std::string input);
template std::string AddActivationNode<math::Tensor<int64_t>>(ActivationType                type,
                                                              Graph<math::Tensor<int64_t>> *g,
                                                              std::string name, std::string input);
template std::string AddActivationNode<math::Tensor<float>>(ActivationType              type,
                                                            Graph<math::Tensor<float>> *g,
                                                            std::string name, std::string input);
template std::string AddActivationNode<math::Tensor<double>>(ActivationType               type,
                                                             Graph<math::Tensor<double>> *g,
                                                             std::string name, std::string input);
template std::string AddActivationNode<math::Tensor<fixed_point::fp32_t>>(
    ActivationType type, Graph<math::Tensor<fixed_point::fp32_t>> *g, std::string name,
    std::string input);
template std::string AddActivationNode<math::Tensor<fixed_point::fp64_t>>(
    ActivationType type, Graph<math::Tensor<fixed_point::fp64_t>> *g, std::string name,
    std::string input);
template std::string AddActivationNode<math::Tensor<fixed_point::fp128_t>>(
    ActivationType type, Graph<math::Tensor<fixed_point::fp128_t>> *g, std::string name,
    std::string input);

template OperationsCount GetActivationCharge<math::Tensor<int8_t>>(ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<int16_t>>(ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<int32_t>>(ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<int64_t>>(ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<float>>(ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<double>>(ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<fixed_point::fp32_t>>(
    ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<fixed_point::fp64_t>>(
    ActivationType type);
template OperationsCount GetActivationCharge<math::Tensor<fixed_point::fp128_t>>(
    ActivationType type);

}  // namespace details
}  // namespace ml
}  // namespace fetch
