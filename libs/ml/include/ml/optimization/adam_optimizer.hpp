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
                DataType const &beta1 = DataType{0.9f}, DataType const &beta2 = DataType{0.999f})
    : Optimizer<T, C>(graph, input_node_name, output_node_name, learning_rate)
    , beta1_(beta1)
    , beta2_(beta2)
    , beta1_t_(beta1)
    , beta2_t_(beta2)
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
    SizeType i{0};
    for (auto &grad : gradients)
    {
      auto git = grad.begin();
      auto cit = cache_[i].begin();
      auto mit = momentum_[i].begin();

      DataType mt;
      DataType vt;
      while (git.is_valid())
      {
        *cit = (beta1_t_ * (*cit)) + ((DataType{1} - beta1_t_) * (*git));
        mt   = *cit / (DataType{1} - beta1_t_);
        *mit = (beta2_t_ * (*mit)) + ((DataType{1} - beta2_t_) * (*git) * (*git));
        vt   = *mit / (DataType{1} - beta2_t_);
        // 1e-4 is added to prevent division by 0
        *git = -this->learning_rate_ * mt / (fetch::math::Sqrt(vt) + DataType{1e-4f});
        ++git;
        ++cit;
        ++mit;
      }
      i++;
    }
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
};

}  // namespace ml
}  // namespace fetch