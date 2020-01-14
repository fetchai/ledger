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

#include "ml/optimisation/adam_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

template <class T>
void AdamOptimiser<T>::Init()
{
  for (auto &train : this->graph_trainables_)
  {
    this->cache_.emplace_back(TensorType(train->GetWeights().shape()));
    this->momentum_.emplace_back(TensorType(train->GetWeights().shape()));
    this->mt_.emplace_back(TensorType(train->GetWeights().shape()));
    this->vt_.emplace_back(TensorType(train->GetWeights().shape()));
  }
  ResetCache();
}

template <class T>
AdamOptimiser<T>::AdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                                std::vector<std::string> const &input_node_names,
                                std::string const &             label_node_name,
                                std::string const &output_node_name, DataType const &learning_rate,
                                DataType const &beta1, DataType const &beta2,
                                DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate)
  , beta1_(beta1)
  , beta2_(beta2)
  , beta1_t_(beta1)
  , beta2_t_(beta2)
  , epsilon_(epsilon)
{
  Init();
}

template <class T>
AdamOptimiser<T>::AdamOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
    DataType const &beta1, DataType const &beta2, DataType const &epsilon)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate_param)
  , beta1_(beta1)
  , beta2_(beta2)
  , beta1_t_(beta1)
  , beta2_t_(beta2)
  , epsilon_(epsilon)
{
  Init();
}

// private

template <class T>
void AdamOptimiser<T>::ApplyGradients(SizeType batch_size)
{
  // Do operation with gradient
  auto cached_weight_it = cache_.begin();
  auto momentum_it      = momentum_.begin();
  auto mt_it            = mt_.begin();
  auto vt_it            = vt_.begin();

  auto gradient_it  = this->gradients_.begin();
  auto trainable_it = this->graph_trainables_.begin();

  // beta1_t=beta1^t and beta2_t=beta2^t, where t is number of epochs + 1
  fetch::math::Pow(beta1_, static_cast<DataType>(this->epoch_ + 1), beta1_t_);
  fetch::math::Pow(beta2_, static_cast<DataType>(this->epoch_ + 1), beta2_t_);

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {

      // cache[i] = (beta1_t_ * cache[i]) + ((1.0 - beta1_t_) * (input_gradients[i]/batch_size));
      fetch::math::Multiply((*trainable_it)->GetGradientsReferences(),
                            (DataType{1} - beta1_t_) / static_cast<DataType>(batch_size),
                            *gradient_it);
      fetch::math::Multiply(*cached_weight_it, beta1_t_, *cached_weight_it);
      fetch::math::Add(*cached_weight_it, *gradient_it, *cached_weight_it);

      // mt   = cache[i] / (1.0 - beta1_t_);
      fetch::math::Divide(*cached_weight_it, (DataType{1} - beta1_t_), *mt_it);

      // momentum[i] = (beta2_t_ * momentum[i]) + ((1.0 - beta2_t_) *
      // ((input_gradients[i]/batch_size)^2));
      fetch::math::Divide((*trainable_it)->GetGradientsReferences(),
                          static_cast<DataType>(batch_size), *vt_it);
      fetch::math::Square(*vt_it, *vt_it);
      fetch::math::Multiply(*vt_it, (DataType{1} - beta2_t_), *vt_it);
      fetch::math::Multiply(*momentum_it, beta2_t_, *momentum_it);
      fetch::math::Add(*momentum_it, *vt_it, *momentum_it);

      // vt   = momentum[i] / (1.0 - beta2_t_);
      fetch::math::Divide(*momentum_it, (DataType{1} - beta2_t_), *vt_it);

      // output_gradients[i] = -this->learning_rate_ * mt / (sqrt(vt) + epsilon_);
      fetch::math::Sqrt(*vt_it, *gradient_it);
      fetch::math::Add(*gradient_it, epsilon_, *gradient_it);
      fetch::math::Divide(*mt_it, *gradient_it, *gradient_it);
      fetch::math::Multiply(*gradient_it, -this->learning_rate_, *gradient_it);

      // we need to explicitly reset the gradients for this shared op to avoid double counting
      // in the case of shared ops
      (*trainable_it)->ResetGradients();
    }

    ++cached_weight_it;
    ++momentum_it;
    ++mt_it;
    ++vt_it;

    ++gradient_it;
    ++trainable_it;
  }

  // calling apply gradients on the graph ensures that the node caches are reset properly
  this->graph_->ApplyGradients(this->gradients_);
}

template <class T>
void AdamOptimiser<T>::ResetCache()
{
  for (auto &val : this->cache_)
  {
    val.Fill(DataType{0});
  }
  for (auto &moment : this->momentum_)
  {
    moment.Fill(DataType{0});
  }
  beta1_t_ = beta1_;
  beta2_t_ = beta2_;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO (ML-464)
// template class AdamOptimiser<math::Tensor<std::int8_t>>;
template class AdamOptimiser<math::Tensor<std::int16_t>>;
template class AdamOptimiser<math::Tensor<std::int32_t>>;
template class AdamOptimiser<math::Tensor<std::int64_t>>;
template class AdamOptimiser<math::Tensor<float>>;
template class AdamOptimiser<math::Tensor<double>>;
template class AdamOptimiser<math::Tensor<fixed_point::fp32_t>>;
template class AdamOptimiser<math::Tensor<fixed_point::fp64_t>>;
template class AdamOptimiser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
