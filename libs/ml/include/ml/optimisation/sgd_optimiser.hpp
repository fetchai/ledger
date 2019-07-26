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

#include "ml/graph.hpp"
#include "optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Stochastic Gradient Descent optimiser
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T>
class SGDOptimiser : public Optimiser<T>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  SGDOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
               std::string const &label_node_name, std::string const &output_node_name,
               DataType const &learning_rate = static_cast<DataType>(0.001f));

  SGDOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
               std::string const &label_node_name, std::string const &output_node_name,
               fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param);

  ~SGDOptimiser() override = default;


  template <typename S>
  friend void Serialize(S &serializer, SGDOptimiser<T> const &t)
  {

    // serialize parent class first
    auto base_pointer = std::make_shared<Optimiser<T>>(t);
    base_pointer.Serialize(serializer, *base_pointer);

    // sgd optimiser has no member variables to serialise
  }

  template <typename S>
  friend void Deserialize(S &serializer, SGDOptimiser<T> &t)
  {

    auto base_pointer = std::make_shared<Optimiser<T>>(t);
    base_pointer.Deserialize(serializer, *base_pointer);

    // sgd optimiser has no member variables to deserialise
  }

private:
  void ApplyGradients(SizeType batch_size) override;
};

/**
 * Constructor for SGD optimiser in which the user specifies only the fixed learning rate
 * @tparam T
 */
template <class T>
SGDOptimiser<T>::SGDOptimiser(std::shared_ptr<Graph<T>>       graph,
                              std::vector<std::string> const &input_node_names,
                              std::string const &             label_node_name,
                              std::string const &output_node_name, DataType const &learning_rate)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate)
{}

/**
 * Constructor for SGD optimiser in which the user specifies the LearningRateParam object
 * @tparam T
 */
template <class T>
SGDOptimiser<T>::SGDOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<SGDOptimiser<T>::DataType> const &learning_rate_param)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate_param)
{}

// private

template <class T>
void SGDOptimiser<T>::ApplyGradients(SizeType batch_size)
{
  auto trainable_it = this->graph_trainables_.begin();
  auto gradient_it  = this->gradients_.begin();

  // this part of the computation does not change within the while loop, so execute it once
  DataType neg_learning_rate_div_batch_size = (-this->learning_rate_) / static_cast<DataType>(batch_size);

  while (gradient_it != this->gradients_.end())
  {
    // output_grad[i] = (input_grad[i] / batch_size) * -learning_rate
    fetch::math::Multiply((*trainable_it)->get_gradients(), neg_learning_rate_div_batch_size, *gradient_it);

    // Apply gradient weights[i]+=output_grad[i]
    (*trainable_it)->ApplyGradient(*gradient_it);

    ++trainable_it;
    ++gradient_it;
  }
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
