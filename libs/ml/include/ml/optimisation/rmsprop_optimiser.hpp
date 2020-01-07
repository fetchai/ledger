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

#include "math/standard_functions/sqrt.hpp"
#include "ml/core/graph.hpp"
#include "optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Root Mean Square Propagation optimiser explained in paper:
 * https://www.cs.toronto.edu/~tijmen/csc321/slides/lecture_slides_lec6.pdf
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class RMSPropOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;

  RMSPropOptimiser(std::shared_ptr<Graph<T>>       graph,
                   std::vector<std::string> const &input_node_names,
                   std::string const &label_node_name, std::string const &output_node_name,
                   DataType const &learning_rate = fetch::math::Type<DataType>("0.001"),
                   DataType const &decay_rate    = fetch::math::Type<DataType>("0.9"),
                   DataType const &epsilon       = fetch::math::Type<DataType>("0.00000001"));

  RMSPropOptimiser(std::shared_ptr<Graph<T>>       graph,
                   std::vector<std::string> const &input_node_names,
                   std::string const &label_node_name, std::string const &output_node_name,
                   fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                   DataType const &decay_rate = fetch::math::Type<DataType>("0.9"),
                   DataType const &epsilon    = fetch::math::Type<DataType>("0.00000001"));

  ~RMSPropOptimiser() override = default;

  OptimiserType OptimiserCode() override
  {
    return OptimiserType::RMSPROP;
  }

private:
  std::vector<TensorType> cache_;
  DataType                decay_rate_;
  DataType                one_{1};
  DataType                epsilon_;

  void ApplyGradients(SizeType batch_size) override;
  void ResetCache();
  void Init();
};

template <class T>
void RMSPropOptimiser<T>::Init()
{
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(TensorType(train->GetWeights().shape()));
  }

  ResetCache();
}

template <class T>
RMSPropOptimiser<T>::RMSPropOptimiser(std::shared_ptr<Graph<T>>       graph,
                                      std::vector<std::string> const &input_node_names,
                                      std::string const &             label_node_name,
                                      std::string const &             output_node_name,
                                      DataType const &learning_rate, DataType const &decay_rate,
                                      DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate)
  , decay_rate_(decay_rate)
  , epsilon_(epsilon)
{
  Init();
}

template <class T>
RMSPropOptimiser<T>::RMSPropOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
    DataType const &decay_rate, DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate_param)
  , decay_rate_(decay_rate)
  , epsilon_(epsilon)
{
  Init();
}

// private

template <class T>
void RMSPropOptimiser<T>::ApplyGradients(SizeType batch_size)
{
  // Do operation with gradient
  auto cached_weight_it = cache_.begin();
  auto gradient_it      = this->gradients_.begin();
  auto trainable_it     = this->graph_trainables_.begin();

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {

      // cache[i] = decay_rate * cache[i] + (1 - decay_rate) * ((input_grad[i]/batch_size)^2)
      fetch::math::Divide((*trainable_it)->GetGradientsReferences(),
                          static_cast<DataType>(batch_size), *gradient_it);
      fetch::math::Square(*gradient_it, *gradient_it);

      fetch::math::Multiply(*gradient_it, (one_ - decay_rate_), *gradient_it);
      fetch::math::Multiply(*cached_weight_it, decay_rate_, *cached_weight_it);
      fetch::math::Add(*cached_weight_it, *gradient_it, *cached_weight_it);

      // epsilon is added to prevent division by 0
      // output_grad[i] = learning_rate * (input_grad[i]/batch_size) / (sqrt(cache[i]) + epsilon)
      fetch::math::Sqrt(*cached_weight_it, *gradient_it);
      fetch::math::Add(*gradient_it, epsilon_, *gradient_it);
      fetch::math::Divide((*trainable_it)->GetGradientsReferences(), *gradient_it, *gradient_it);
      fetch::math::Multiply(*gradient_it,
                            (-this->learning_rate_) / (static_cast<DataType>(batch_size)),
                            *gradient_it);

      // we need to explicitly reset the gradients for this shared op to avoid double counting
      // in the case of shared ops
      (*trainable_it)->ResetGradients();
    }

    ++cached_weight_it;
    ++gradient_it;
    ++trainable_it;
  }

  // calling apply gradients on the graph ensures that the node caches are reset properly
  this->graph_->ApplyGradients(this->gradients_);
}

template <class T>
void RMSPropOptimiser<T>::ResetCache()
{
  for (auto &val : this->cache_)
  {
    val.Fill(DataType{0});
  }
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
