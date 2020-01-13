#pragma once
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
  using SizeType   = fetch::math::SizeType;
  using SizeSet    = std::unordered_set<SizeType>;

  SGDOptimiser() = default;
  SGDOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
               std::string const &label_node_name, std::string const &output_node_name,
               DataType const &learning_rate = fetch::math::Type<DataType>("0.001"));

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
  // ApplyGradientSparse if number_of_rows_to_update * sparsity_threshold_ <= total_rows
  SizeType sparsity_threshold_ = 2;

  void ApplyGradients(SizeType batch_size) override;
};

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
