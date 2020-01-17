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

#include "math/fundamental_operators.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/trainable.hpp"
#include "ml/optimisation/adagrad_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

template <class T>
AdaGradOptimiser<T>::AdaGradOptimiser(std::shared_ptr<Graph<T>>       graph,
                                      std::vector<std::string> const &input_node_names,
                                      std::string const &             label_node_name,
                                      std::string const &             output_node_name,
                                      DataType const &learning_rate, DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate)
  , epsilon_(epsilon)
{
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(TensorType(train->GetWeights().shape()));
  }
  ResetCache();
}

template <class T>
AdaGradOptimiser<T>::AdaGradOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
    DataType const &                                          epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate_param)
  , epsilon_(epsilon)
{
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(TensorType(train->GetWeights().shape()));
  }
  ResetCache();
}

// private

template <class T>
void AdaGradOptimiser<T>::ApplyGradients(SizeType batch_size)
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

      // cache[i] += (input_grad[i]/batch_size)^2
      fetch::math::Divide((*trainable_it)->GetGradientsReferences(),
                          static_cast<DataType>(batch_size), *gradient_it);
      fetch::math::Square(*gradient_it, *gradient_it);
      fetch::math::Add(*cached_weight_it, *gradient_it, *cached_weight_it);

      // epsilon is added to prevent division by 0
      // output_grad[i] = learning_rate * (grad[i]/batch_size) / (sqrt(cache[i]) + epsilon)
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
void AdaGradOptimiser<T>::ResetCache()
{
  for (auto &val : this->cache_)
  {
    val.Fill(DataType{0});
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO (ML-464)
// template class AdaGradOptimiser<math::Tensor<std::int8_t>>;
template class AdaGradOptimiser<math::Tensor<std::int16_t>>;
template class AdaGradOptimiser<math::Tensor<std::int32_t>>;
template class AdaGradOptimiser<math::Tensor<std::int64_t>>;
template class AdaGradOptimiser<math::Tensor<float>>;
template class AdaGradOptimiser<math::Tensor<double>>;
template class AdaGradOptimiser<math::Tensor<fixed_point::fp32_t>>;
template class AdaGradOptimiser<math::Tensor<fixed_point::fp64_t>>;
template class AdaGradOptimiser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
