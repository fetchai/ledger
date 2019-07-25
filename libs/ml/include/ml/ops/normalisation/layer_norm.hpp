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

#include "math/activation_functions/leaky_relu.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/statistics/standard_deviation.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LayerNorm : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  explicit LayerNorm()  = default;
  ~LayerNorm() override = default;

  /**
   * Forward pass consists of calculating mean and standard deviation, mean-covariance shift
   * normalising, and then linearly transforming with trainable parameters gamma and beta
   * @param inputs
   * @param output
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    // input tensor, gamma, beta
    assert(inputs.size() == 3);

    // TODO - different behaviour if IsTraining is false

    // compute mean and standard deviations along batch dimension
    auto     batch_dim = inputs.at(0).get().shape().size() - 1;
    DataType mean;
    fetch::math::ReduceMean(inputs.at(0).get(), batch_dim, mean);
    DataType std_dev = fetch::math::statistics::StandardDeviation(inputs.at(0).get());

    // forward batch normalise
    for (SizeType i = 0; i < inputs.at(0).get().shape(batch_dim); ++i)
    {
      auto t_it = inputs.at(0).get().View(i).cbegin();
      auto r_it = output.at(0).get().View(i).begin();

      auto g_it = inputs.at(1).get().cbegin();
      auto b_it = inputs.at(2).get().cbegin();

      while (t_it.is_valid)
      {
        // mean covariance shift normalisation (plus epsilon to avoid / 0 )
        *r_it = ((*t_it) - mean) / (std_dev + fetch::math::numeric_lowest<DataType>());

        // multiply by gamma trainable param
        *r_it *= *g_it;

        // plus beta trainable param
        *r_it += *b_it;

        ++r_it;
        ++g_it;
        ++b_it;
        ++t_it;
      }
    }
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {

    SizeType batch_dimension = inputs.at(0).get().shape().size() - 1;

    ArrayType beta_err_signal({error_signal.shape(0), 1});
    fetch::math::Multiply(inputs.at(0).get(), error_signal, beta_err_signal);

    ArrayType beta_err_signal({error_signal.shape(0), 1});
    fetch::math::ReduceSum(error_signal, batch_dimension, beta_err_signal);

    return {error_signal, gamma_err_signal, beta_err_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "BatchNorm";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
