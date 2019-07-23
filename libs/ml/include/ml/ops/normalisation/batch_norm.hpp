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
class BatchNorm : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  explicit BatchNorm() = default;
  ~BatchNorm() override = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    // input tensor, gamma, beta
    assert(inputs.size() == 3);

    // TODO - different behaviour if IsTraining is false

    // compute mean and standard deviations along batch dimension
    auto batch_dim = inputs.at(0).get().shape().size() - 1;
    DataType mean;
    fetch::math::ReduceMean(inputs.at(0).get(), batch_dim, mean);
    DataType std_dev = fetch::math::statistics::StandardDeviation(inputs.at(0).get());

    // forward batch normalise
    auto t_it = inputs.at(0).get().cbegin();
    for (SizeType i = 0; i < ipnuts.at(0).get().shape(batch_dim); ++SizeType i)
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
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == error_signal.shape());
    DataType  zero{0};
    DataType  one{1};
    ArrayType ret{error_signal.shape()};
    ArrayType t{inputs.front().get().shape()};

    // gradient of leaky relu function is a where x<0; and 1.0 where x>=0
    this->Forward(inputs, t);

    auto it  = t.cbegin();
    auto rit = ret.begin();
    while (it.is_valid())
    {
      if (*it >= zero)
      {
        // f'(x)=1 for x>=0
        *rit = one;
      }
      else
      {
        // f'(x)=a for x<0
        *rit = a_;
      }
      ++it;
      ++rit;
    }

    // multiply by error_signal (chain rule)
    fetch::math::Multiply(error_signal, ret, ret);

    return {ret};
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
