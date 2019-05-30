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

#include "math/standard_functions/sqrt.hpp"
#include "ml/graph.hpp"
#include "ml/ops/loss_functions/criterion.hpp"
#include "ml/optimization/optimizer.hpp"

namespace fetch {
namespace ml {

/**
 * Adaptive Momentum optimizer
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class AdamOptimizer : public Optimizer<T, C>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  AdamOptimizer(std::shared_ptr<Graph<T>> graph, std::string const &input_node_name,
                std::string const &output_node_name, DataType const &learning_rate,
                DataType const &beta1 = DataType{0.9f}, DataType const &beta2 = DataType{0.999f},
                DataType const &epsilon = DataType{1e-4f})
    : Optimizer<T, C>(graph, input_node_name, output_node_name, learning_rate)
    , beta1_(beta1)
    , beta2_(beta2)
    , beta1_t_(beta1)
    , beta2_t_(beta2)
    , epsilon_(epsilon)

  {
    auto weights = this->graph_->GetWeights();
    for (auto &wei : weights)
    {
      this->cache_.push_back(ArrayType(wei.shape()));
      this->momentum_.push_back(ArrayType(wei.shape()));
    }
    ResetCache();
  }

private:
  void ApplyGradients()
  {
    std::vector<ArrayType> gradients = this->graph_->GetGradients();

    // Do operation with gradient
    auto cached_weight_it = cache_.begin();
    auto momentum_it      = momentum_.begin();
    for (auto &grad : gradients)
    {
      auto git = grad.begin();
      auto cit = (*cached_weight_it).begin();
      auto mit = (*momentum_it).begin();

      DataType mt;
      DataType vt;
      while (git.is_valid())
      {
        *cit = (beta1_t_ * (*cit)) + ((one_ - beta1_t_) * (*git));
        mt   = *cit / (one_ - beta1_t_);
        *mit = (beta2_t_ * (*mit)) + ((one_ - beta2_t_) * (*git) * (*git));
        vt   = *mit / (one_ - beta2_t_);
        // epsilon is added to prevent division by 0
        *git = -this->learning_rate_ * mt / (fetch::math::Sqrt(vt) + epsilon_);
        ++git;
        ++cit;
        ++mit;
      }
      ++cached_weight_it;
      ++momentum_it;
    }
    // weights[i]+=grad[i]
    this->graph_->ApplyGradients(gradients);

    // beta1_t=beta1^t and beta2_t=beta2^t, where t is number of epochs + 1
    beta1_t_ *= beta1_;
    beta2_t_ *= beta2_;
  }

  void ResetCache()
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

  std::vector<ArrayType> cache_;
  std::vector<ArrayType> momentum_;
  DataType               beta1_;
  DataType               beta2_;
  DataType               beta1_t_;
  DataType               beta2_t_;
  DataType               epsilon_;
  DataType               one_{1};
};

}  // namespace ml
}  // namespace fetch