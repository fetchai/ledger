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

#include "ml/core/graph.hpp"
#include "optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Stochastic Gradient Descent optimiser
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class SGDOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = typename TensorType::SizeType;

  SGDOptimiser() = default;
  SGDOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
               std::string const &label_node_name, std::string const &output_node_name,
               DataType const &learning_rate = static_cast<DataType>(0.001f));

  SGDOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
               std::string const &label_node_name, std::string const &output_node_name,
               fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param);

  ~SGDOptimiser() override = default;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::SGD;
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
  DataType neg_learning_rate_div_batch_size =
      (-this->learning_rate_) / static_cast<DataType>(batch_size);

  while (gradient_it != this->gradients_.end())
  {
    // output_grad[i] = (input_grad[i] / batch_size) * -learning_rate
    fetch::math::Multiply((*trainable_it)->GetGradientsReferences(),
                          neg_learning_rate_div_batch_size, *gradient_it);

    // Apply gradient weights[i]+=output_grad[i]
    (*trainable_it)->ApplyGradient(*gradient_it);

    ++trainable_it;
    ++gradient_it;
  }
}

}  // namespace optimisers
}  // namespace ml

namespace serializers {
/**
 * serializer for SGDOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::SGDOptimiser<TensorType>, D>
{
  using Type                          = ml::optimisers::SGDOptimiser<TensorType>;
  using DriverType                    = D;
  static uint8_t const BASE_OPTIMISER = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> const *>(&sp);
    map.Append(BASE_OPTIMISER, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPTIMISER, *base_pointer);
  }
};
}  // namespace serializers

}  // namespace fetch
