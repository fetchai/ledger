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

#include "ml/charge_estimation/constants.hpp"
#include "ml/charge_estimation/optimisation/constants.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/trainable.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

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

template <class T>
OperationsCount SGDOptimiser<T>::ChargeConstruct(std::shared_ptr<Graph<T>> graph)
{
  auto trainables = graph->GetTrainables();

  OperationsCount op_cnt{charge_estimation::FUNCTION_CALL_COST};
  for (auto &train : trainables)
  {
    auto weight_shape = train->GetFutureDataShape();
    if (weight_shape.empty())
    {
      throw std::runtime_error("Shape deduction failed");
    }

    SizeType data_size = TensorType::PaddedSizeFromShape(weight_shape);
    op_cnt += data_size * charge_estimation::optimisers::SGD_N_CACHES;
  }

  return op_cnt;
}

template <class T>
fetch::ml::OperationsCount SGDOptimiser<T>::ChargeStep() const
{
  auto gradient_it  = this->gradients_.begin();
  auto trainable_it = this->graph_trainables_.begin();

  // Update betas, initialise
  OperationsCount ops_count = charge_estimation::optimisers::SGD_STEP_INIT;

  OperationsCount loop_count{0};
  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {
      loop_count += T::SizeFromShape((*trainable_it)->GetWeights().shape());
    }

    // Skip frozen trainables
    if (!(*trainable_it)->GetFrozenState())
    {

      auto gradient_pair = (*trainable_it)->GetSparseGradientsReferences();
      loop_count += charge_estimation::optimisers::SGD_PER_TRAINABLE;

      // Normal ApplyGradient
      if (gradient_pair.second.empty() ||
          (gradient_pair.second.size() * sparsity_threshold_) > gradient_pair.first.shape().at(1))
      {
        loop_count += T::SizeFromShape((*trainable_it)->GetWeights().shape());
      }
      else
      {
        // Sparse apply gradient
        std::vector<SizeType> slice_size = gradient_it->shape();
        slice_size.at(0)                 = 1;

        loop_count += gradient_pair.second.size() * T::SizeFromShape(slice_size) *
                      charge_estimation::optimisers::SGD_SPARSE_APPLY;
      }

      // ResetGradients();
      loop_count += T::SizeFromShape((*trainable_it)->GetWeights().shape());
    }

    ++gradient_it;
    ++trainable_it;
  }

  return ops_count + loop_count;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO (ML-464)
// template class SGDOptimiser<math::Tensor<std::int8_t>>;
// template class SGDOptimiser<math::Tensor<std::int16_t>>;
template class SGDOptimiser<math::Tensor<std::int32_t>>;
template class SGDOptimiser<math::Tensor<std::int64_t>>;
template class SGDOptimiser<math::Tensor<float>>;
template class SGDOptimiser<math::Tensor<double>>;
template class SGDOptimiser<math::Tensor<fixed_point::fp32_t>>;
template class SGDOptimiser<math::Tensor<fixed_point::fp64_t>>;
template class SGDOptimiser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
