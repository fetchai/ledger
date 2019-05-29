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
 * Root Mean Square Propagation optimizer explained in paper:
 * https://www.cs.toronto.edu/~tijmen/csc321/slides/lecture_slides_lec6.pdf
 * @tparam T ArrayType
 * @tparam C CriterionType
 */
template <class T, class C>
class RMSPropOptimizer : public Optimizer<T, C>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  RMSPropOptimizer(std::shared_ptr<Graph<T>> graph, std::string const &input_node_name,
                   std::string const &output_node_name, DataType const &learning_rate,
                   DataType const &decay_rate = DataType{0.9f})
    : Optimizer<T, C>(graph, input_node_name, output_node_name, learning_rate)
    , decay_rate_(decay_rate)
  {
    auto weights = this->graph_->GetWeights();
    for (auto &wei : weights)
    {
      this->cache_.push_back(ArrayType(wei.shape()));
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
      while (git.is_valid())
      {
        *cit = decay_rate_ * (*cit) + (DataType{1} - decay_rate_) * (*git) * (*git);
        // 1e-8 is added to prevent division by 0
        *git = -this->learning_rate_ * ((*git) / fetch::math::Sqrt(*cit + DataType{1e-8f}));
        ++git;
        ++cit;
      }
      i++;
    }
    this->graph_->ApplyGradients(gradients);
  }

  void ResetCache()
  {
    for (auto &val : this->cache_)
    {
      val.Fill(DataType{0});
    }
  }

  std::vector<ArrayType> cache_;
  DataType               decay_rate_;
};

}  // namespace ml
}  // namespace fetch