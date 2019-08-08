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
#include "ml/optimisation/optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

/**
 * Adaptive Momentum optimiser
 * i.e.  Stochastic gradient descent with momentum
 * @tparam T TensorType
 * @tparam C CriterionType
 */
template <class T>
class MomentumOptimiser : public Optimiser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = typename TensorType::SizeType;

  MomentumOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    DataType const &learning_rate   = DataType{0.001f},
                    DataType const &momentum_update = DataType{0.9f});

  MomentumOptimiser(std::shared_ptr<Graph<T>>       graph,
                    std::vector<std::string> const &input_node_names,
                    std::string const &label_node_name, std::string const &output_node_name,
                    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
                    DataType const &momentum_update = DataType{0.9f});

  ~MomentumOptimiser() override = default;

private:
  std::vector<TensorType> momentum_;
  DataType                momentum_update_;
  DataType                negative_one_{-1};
  DataType                zero_{0};

  void ApplyGradients(SizeType batch_size) override;
  void ResetMomentum();
  void Init();
};

template <class T>
void MomentumOptimiser<T>::Init()
{
  for (auto &train : this->graph_trainables_)
  {
    this->momentum_.emplace_back(TensorType(train->get_weights().shape()));
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
    // momentum[i] = momentum_update * momentum[i] + learning_rate * (input_grad[i]/batch_size)
    fetch::math::Multiply(*mit, momentum_update_, *mit);
    fetch::math::Multiply((*trainable_it)->get_gradients(),
                          (this->learning_rate_) / (static_cast<DataType>(batch_size)),
                          *gradient_it);
    fetch::math::Add(*mit, *gradient_it, *mit);

    // output_grad[i]=-momentum[i]
    fetch::math::Multiply(*mit, negative_one_, *gradient_it);

    // Apply gradient weights[i]+=output_grad[i]
    (*trainable_it)->ApplyGradient(*gradient_it);

    ++trainable_it;
    ++gradient_it;
    ++mit;
  }
}

template <class T>
void MomentumOptimiser<T>::ResetMomentum()
{
  for (auto &moment : this->momentum_)
  {
    moment.Fill(zero_);
  }
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
