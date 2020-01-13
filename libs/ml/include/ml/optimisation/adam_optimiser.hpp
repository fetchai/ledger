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
#include "ml/optimisation/optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Adaptive Momentum optimiser
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class AdamOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;

  AdamOptimiser() = default;

  AdamOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
                std::string const &label_node_name, std::string const &output_node_name,
                DataType const &learning_rate = fetch::math::Type<DataType>("0.001"),
                DataType const &beta1         = fetch::math::Type<DataType>("0.9"),
                DataType const &beta2         = fetch::math::Type<DataType>("0.999"),
                DataType const &epsilon       = fetch::math::Type<DataType>("0.0001"));

  AdamOptimiser(std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
                std::string const &label_node_name, std::string const &output_node_name,
                fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                DataType const &beta1   = fetch::math::Type<DataType>("0.9"),
                DataType const &beta2   = fetch::math::Type<DataType>("0.999"),
                DataType const &epsilon = fetch::math::Type<DataType>("0.0001"));

  ~AdamOptimiser() override = default;

  void ApplyGradients(SizeType batch_size) override;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::ADAM;
  }

protected:
  std::vector<TensorType> cache_;
  std::vector<TensorType> momentum_;
  std::vector<TensorType> mt_;
  std::vector<TensorType> vt_;

  DataType beta1_;
  DataType beta2_;
  DataType beta1_t_;
  DataType beta2_t_;
  DataType epsilon_;

  void ResetCache();
  void Init();
};

}  // namespace optimisers
}  // namespace ml

namespace serializers {
/**
 * serializer for SGDOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::AdamOptimiser<TensorType>, D>
{
  using Type                          = ml::optimisers::AdamOptimiser<TensorType>;
  using DriverType                    = D;
  static uint8_t const BASE_OPTIMISER = 1;
  static uint8_t const CACHE          = 2;
  static uint8_t const MOMENTUM       = 3;
  static uint8_t const MT             = 4;
  static uint8_t const VT             = 5;
  static uint8_t const BETA1          = 6;
  static uint8_t const BETA2          = 7;
  static uint8_t const BETA1_T        = 8;
  static uint8_t const BETA2_T        = 9;
  static uint8_t const EPSILON        = 10;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(10);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> const *>(&sp);
    map.Append(BASE_OPTIMISER, *base_pointer);

    map.Append(CACHE, sp.cache_);
    map.Append(MOMENTUM, sp.momentum_);
    map.Append(MT, sp.mt_);
    map.Append(VT, sp.vt_);
    map.Append(BETA1, sp.beta1_);
    map.Append(BETA2, sp.beta2_);
    map.Append(BETA1_T, sp.beta1_t_);
    map.Append(BETA2_T, sp.beta2_t_);
    map.Append(EPSILON, sp.epsilon_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPTIMISER, *base_pointer);

    map.ExpectKeyGetValue(CACHE, sp.cache_);
    map.ExpectKeyGetValue(MOMENTUM, sp.momentum_);
    map.ExpectKeyGetValue(MT, sp.mt_);
    map.ExpectKeyGetValue(VT, sp.vt_);
    map.ExpectKeyGetValue(BETA1, sp.beta1_);
    map.ExpectKeyGetValue(BETA2, sp.beta2_);
    map.ExpectKeyGetValue(BETA1_T, sp.beta1_t_);
    map.ExpectKeyGetValue(BETA2_T, sp.beta2_t_);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon_);
  }
};
}  // namespace serializers

}  // namespace fetch
