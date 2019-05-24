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

#include "ml/graph.hpp"
#include "ml/ops/loss_functions/criterion.hpp"
#include "ml/optimization/optimizer.hpp"

namespace fetch {
namespace ml {

template <class T>
class SGDOptimizer : public Optimizer<T>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;
  using SizeType  = typename ArrayType::SizeType;

  SGDOptimizer(std::shared_ptr<Graph<T>> graph, std::shared_ptr<ops::Criterion<T>> criterion,
               std::string const &input_node_name, std::string const &output_node_name,
               DataType const &learning_rate)
    : Optimizer<T>(graph, criterion, input_node_name, output_node_name, learning_rate)
  {}

private:
  void ApplyGradients()
  {
    std::vector<ArrayType> gradients = this->graph_->GetGradients();

    // Do operation with gradient
    SizeType i{0};
    for (auto &grad : gradients)
    {
      fetch::math::Multiply(grad, -this->learning_rate_, grad);
      i++;
    }
    this->graph_->ApplyGradients(gradients);
  }
};

}  // namespace ml
}  // namespace fetch