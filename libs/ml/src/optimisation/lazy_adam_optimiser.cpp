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
#include "math/base_types.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"
#include "ml/ops/trainable.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

template <class T>
LazyAdamOptimiser<T>::LazyAdamOptimiser(std::shared_ptr<Graph<T>>       graph,
                                        std::vector<std::string> const &input_node_names,
                                        std::string const &             label_node_name,
                                        std::string const &             output_node_name,
                                        DataType const &learning_rate, DataType const &beta1,

                                        DataType const &beta2, SizeType sparsity_threshold,
                                        DataType const &epsilon)
  : AdamOptimiser<T>(graph, input_node_names, label_node_name, output_node_name, learning_rate,
                     beta1, beta2, epsilon)
  , sparsity_threshold_(sparsity_threshold)
{}

template <class T>
LazyAdamOptimiser<T>::LazyAdamOptimiser(
    std::shared_ptr<Graph<T>> graph, std::vector<std::string> const &input_node_names,
    std::string const &label_node_name, std::string const &output_node_name,
    fetch::ml::optimisers::LearningRateParam<DataType> const &learning_rate_param,
    DataType const &beta1, DataType const &beta2, SizeType sparsity_threshold,
    DataType const &epsilon)
  : AdamOptimiser<T>(graph, input_node_names, label_node_name, output_node_name,
                     learning_rate_param, beta1, beta2, epsilon)
  , sparsity_threshold_(sparsity_threshold)
{}

// private

/**
 * ApplyLogic does optimiser step
 * i. e. Applies momentum and exponential moving average to gradient tensor
 * @tparam T
 * @param batch_size
 * @param gradient_tensor
 * @param momentum_tensor
 * @param mt_tensor
 * @param v_tensor
 * @param cache_tensor
 * @param refs_tensor
 */
template <class T>
void LazyAdamOptimiser<T>::ApplyLogic(SizeType batch_size, TensorType &gradient_tensor,
                                      TensorType &momentum_tensor, TensorType &mt_tensor,
                                      TensorType &v_tensor, TensorType &cache_tensor,
                                      TensorType const &refs_tensor)
{

  // cache[i] = (beta1_t_ * cache[i]) + ((1.0 - beta1_t_) * (input_gradients[i]/batch_size));
  fetch::math::Multiply(refs_tensor,
                        (DataType{1} - this->beta1_t_) / static_cast<DataType>(batch_size),
                        gradient_tensor);
  fetch::math::Multiply(cache_tensor, this->beta1_t_, cache_tensor);
  fetch::math::Add(cache_tensor, gradient_tensor, cache_tensor);

  // mt   = cache[i] / (1.0 - beta1_t_);
  fetch::math::Divide(cache_tensor, (static_cast<DataType>(1) - this->beta1_t_), mt_tensor);

  // momentum[i] = (beta2_t_ * momentum[i]) + ((1.0 - beta2_t_) *
  // ((input_gradients[i]/batch_size)^2));
  fetch::math::Divide(refs_tensor, static_cast<DataType>(batch_size), v_tensor);
  fetch::math::Square(v_tensor, v_tensor);
  fetch::math::Multiply(v_tensor, (DataType{1} - this->beta2_t_), v_tensor);
  fetch::math::Multiply(momentum_tensor, this->beta2_t_, momentum_tensor);
  fetch::math::Add(momentum_tensor, v_tensor, momentum_tensor);

  // vt   = momentum[i] / (1.0 - beta2_t_);
  fetch::math::Divide(momentum_tensor, (DataType{1} - this->beta2_t_), v_tensor);

  // output_gradients[i] = -this->learning_rate_ * mt / (sqrt(vt) + epsilon_);
  fetch::math::Sqrt(v_tensor, gradient_tensor);
  fetch::math::Add(gradient_tensor, this->epsilon_, gradient_tensor);
  fetch::math::Divide(mt_tensor, gradient_tensor, gradient_tensor);
  fetch::math::Multiply(gradient_tensor, -this->learning_rate_, gradient_tensor);
}

template <class T>
void LazyAdamOptimiser<T>::ApplyGradients(SizeType batch_size)
{
  // Do operation with gradient
  auto cached_weight_it = this->cache_.begin();
  auto momentum_it      = this->momentum_.begin();
  auto mt_it            = this->mt_.begin();
  auto vt_it            = this->vt_.begin();

  auto gradient_it  = this->gradients_.begin();
  auto trainable_it = this->graph_trainables_.begin();

  // beta1_t=beta1^t and beta2_t=beta2^t, where t is number of epochs + 1
  fetch::math::Pow(this->beta1_, static_cast<DataType>(this->epoch_ + 1), this->beta1_t_);
  fetch::math::Pow(this->beta2_, static_cast<DataType>(this->epoch_ + 1), this->beta2_t_);

  std::vector<SizeSet> rows;

  while (gradient_it != this->gradients_.end())
  {
    // Skip frozen trainables
    if ((*trainable_it)->GetFrozenState())
    {
      continue;
    }

    std::pair<TensorType const, SizeSet const> gradient_pair =
        (*trainable_it)->GetSparseGradientsReferences();
    rows.push_back(gradient_pair.second);

    // Normal ApplyGradient
    // if number_of_rows_to_update * sparsity_threshold_ > total_rows
    if (rows.at(rows.size() - 1).empty() ||
        (rows.at(rows.size() - 1).size() * sparsity_threshold_) > gradient_pair.first.shape().at(1))
    {
      ApplyLogic(batch_size, *gradient_it, *momentum_it, *mt_it, *vt_it, *cached_weight_it,
                 (*trainable_it)->GetGradientsReferences());
    }
    // Sparse apply gradient
    // if number_of_rows_to_update * sparsity_threshold_ <= total_rows
    else
    {
      // Tensors used by ApplyLogic for operations that are not possible to do in-place
      TensorType mt_tensor({mt_it->shape().at(0), 1});
      TensorType vt_tensor({vt_it->shape().at(0), 1});

      for (SizeType update_index : rows.at(rows.size() - 1))
      {

        auto       gradient_view        = gradient_it->View(update_index);
        TensorType gradient_view_tensor = gradient_view.Copy();

        auto       momentum_view        = momentum_it->View(update_index);
        TensorType momentum_view_tensor = momentum_view.Copy();

        auto       cache_view        = cached_weight_it->View(update_index);
        TensorType cache_view_tensor = cache_view.Copy();

        TensorType refs_view_tensor =
            (*trainable_it)->GetGradientsReferences().View(update_index).Copy();

        ApplyLogic(batch_size, gradient_view_tensor, momentum_view_tensor, mt_tensor, vt_tensor,
                   cache_view_tensor, refs_view_tensor);

        // Put things back
        gradient_view.Assign(gradient_view_tensor);
        momentum_view.Assign(momentum_view_tensor);
        cache_view.Assign(cache_view_tensor);
      }

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
  this->graph_->ApplySparseGradients(this->gradients_, rows);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// TODO (ML-464)
// template class LazyAdamOptimiser<math::Tensor<std::int8_t>>;
template class LazyAdamOptimiser<math::Tensor<std::int16_t>>;
template class LazyAdamOptimiser<math::Tensor<std::int32_t>>;
template class LazyAdamOptimiser<math::Tensor<std::int64_t>>;
template class LazyAdamOptimiser<math::Tensor<float>>;
template class LazyAdamOptimiser<math::Tensor<double>>;
template class LazyAdamOptimiser<math::Tensor<fixed_point::fp32_t>>;
template class LazyAdamOptimiser<math::Tensor<fixed_point::fp64_t>>;
template class LazyAdamOptimiser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
