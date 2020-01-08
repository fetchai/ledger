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

  std::vector<SizeSet> rows;

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {

      auto gradient_pair = (*trainable_it)->GetSparseGradientsReferences();
      rows.push_back(gradient_pair.second);

      // Normal ApplyGradient
      // if number_of_rows_to_update * sparsity_threshold_ > total_rows
      if (rows.at(rows.size() - 1).empty() ||
          (rows.at(rows.size() - 1).size() * sparsity_threshold_) >
              gradient_pair.first.shape().at(1))
      {

        // output_grad[i] = (input_grad[i] / batch_size) * -learning_rate
        fetch::math::Multiply(gradient_pair.first, neg_learning_rate_div_batch_size, *gradient_it);
      }
      else
      {
        // Sparse apply gradient
        // if number_of_rows_to_update * sparsity_threshold_ <= total_rows

        for (SizeType update_index : rows.at(rows.size() - 1))
        {
          auto       gradient_slice        = gradient_it->View(update_index);
          TensorType gradient_slice_tensor = gradient_slice.Copy();

          auto       refs_slice        = gradient_pair.first.View(update_index);
          TensorType refs_slice_tensor = refs_slice.Copy();

          // output_grad[i] = (input_grad[i] / batch_size) * -learning_rate
          fetch::math::Multiply(refs_slice_tensor, neg_learning_rate_div_batch_size,
                                gradient_slice_tensor);

          gradient_slice.Assign(gradient_slice_tensor);
        }
      }

      // we need to explicitly reset the gradients for this shared op to avoid double counting
      // in the case of shared ops
      (*trainable_it)->ResetGradients();
    }
    ++trainable_it;
    ++gradient_it;
  }

  // calling apply gradients on the graph ensures that the node caches are reset properly
  this->graph_->ApplySparseGradients(this->gradients_, rows);
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
