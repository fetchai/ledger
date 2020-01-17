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
#include "ml/ops/trainable.hpp"
#include "ml/optimisation/momentum_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

template <class T>
void MomentumOptimiser<T>::Init()
{
  for (auto &train : this->graph_trainables_)
  {
    this->momentum_.emplace_back(TensorType(train->GetWeights().shape()));
  }
  ResetMomentum();
}

template <class T>
MomentumOptimiser<T>::MomentumOptimiser(std::shared_ptr<Graph<T>>       graph,
                                        std::vector<std::string> const &input_node_names,
                                        std::string const &             label_node_name,
                                        std::string const &             output_node_name,
                                        DataType const &                learning_rate,
                                        DataType const &                momentum_update)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate)
  , momentum_update_(momentum_update)
{
  Init();
}

template <class T>
MomentumOptimiser<T>::MomentumOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
    DataType const &                                          momentum_update)
  : Optimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate_param)
  , momentum_update_(momentum_update)
{
  Init();
}

// private

template <class T>
void MomentumOptimiser<T>::ApplyGradients(SizeType batch_size)
{
  auto trainable_it = this->graph_trainables_.begin();
  auto gradient_it  = this->gradients_.begin();
  auto mit          = momentum_.begin();

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {

      // momentum[i] = momentum_update * momentum[i] + learning_rate * (input_grad[i]/batch_size)
      fetch::math::Multiply(*mit, momentum_update_, *mit);
      fetch::math::Multiply((*trainable_it)->GetGradientsReferences(),
                            (this->learning_rate_) / (static_cast<DataType>(batch_size)),
                            *gradient_it);
      fetch::math::Add(*mit, *gradient_it, *mit);

      // output_grad[i]=-momentum[i]
      fetch::math::Multiply(*mit, negative_one_, *gradient_it);

      // we need to explicitly reset the gradients for this shared op to avoid double counting
      // in the case of shared ops
      (*trainable_it)->ResetGradients();
    }

    ++trainable_it;
    ++gradient_it;
    ++mit;
  }

  // calling apply gradients on the graph ensures that the node caches are reset properly
  this->graph_->ApplyGradients(this->gradients_);
}

template <class T>
void MomentumOptimiser<T>::ResetMomentum()
{
  for (auto &moment : this->momentum_)
  {
    moment.Fill(zero_);
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO (ML-464)
// template class MomentumOptimiser<math::Tensor<std::int8_t>>;
//template class MomentumOptimiser<math::Tensor<std::int16_t>>;
template class MomentumOptimiser<math::Tensor<std::int32_t>>;
template class MomentumOptimiser<math::Tensor<std::int64_t>>;
template class MomentumOptimiser<math::Tensor<float>>;
template class MomentumOptimiser<math::Tensor<double>>;
template class MomentumOptimiser<math::Tensor<fixed_point::fp32_t>>;
template class MomentumOptimiser<math::Tensor<fixed_point::fp64_t>>;
template class MomentumOptimiser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
