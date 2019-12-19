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

#include "core/macros.hpp"
#include "core/random/lfg.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Dropout : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using RNG           = fetch::random::LaggedFibonacciGenerator<>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpDropoutSaveableParams<TensorType>;
  using MyType        = Dropout<TensorType>;

  explicit Dropout(DataType const probability, SizeType const &random_seed = 25102015)
    : probability_of_keeping_(probability)
  {
    if (probability < zero_ || probability > one_)
    {
      std::stringstream ss;
      ss << probability;
      throw std::runtime_error("Dropout probability " + ss.str() +
                               " is out of allowed range [0..1]");
    }
    rng_.Seed(random_seed);
    scaling_coefficients_ = TensorType{0};
  }

  explicit Dropout(SPType const &sp)
    : Ops<T>(sp)
    , probability_of_keeping_(sp.probability)
  {
    rng_.Seed(sp.random_seed);
    rng_.SetBuffer(sp.buffer);
    rng_.SetIndex(sp.index);
  }

  ~Dropout() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.probability = probability_of_keeping_;
    sp.random_seed = rng_.Seed();
    sp.buffer      = rng_.GetBuffer();
    sp.index       = rng_.GetIndex();
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    if (me.get() != this)
    {
      throw std::runtime_error("Dropout::MakeSharedCopy fatal error: invalid argument.");
    }

    return me;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    if (inputs.size() != 1)
    {
      throw fetch::math::exceptions::WrongShape(
          "Can not do a forward pass through Dropout layer: wrong inputs provided.");
    }

    if (output.shape() != this->ComputeOutputShape(inputs))
    {
      throw fetch::math::exceptions::WrongShape(
          "Can not do a forward pass through Dropout layer: output shape mismatch.");
    }

    if (!this->is_training_ || probability_of_keeping_ == one_)
    {
      output.Copy((*inputs.front()));
      return;
    }

    if (probability_of_keeping_ == zero_)
    {
      output.Fill(zero_);
      scaling_coefficients_.Fill(zero_);
      return;
    }

    if (scaling_coefficients_.shape() != output.shape())
    {
      scaling_coefficients_ = TensorType(output.shape());
    }

    DataType const scale = one_ / probability_of_keeping_;

    auto output_it       = output.begin();
    auto input_it        = inputs.front()->cbegin();
    auto scaling_coef_it = scaling_coefficients_.begin();
    while (scaling_coef_it.is_valid())
    {
      DataType const random_probability = rng_.AsType<DataType>();
      if (random_probability <= probability_of_keeping_)
      {
        *scaling_coef_it = scale;
        *output_it       = scale * (*input_it);
      }
      else
      {
        *scaling_coef_it = zero_;
        *output_it       = zero_;
      }
      ++scaling_coef_it;
      ++input_it;
      ++output_it;
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    if (!this->is_training_)
    {
      throw std::runtime_error(
          "Attempt to do a backward pass in Dropout layer while not in training mode!");
    }
    if (inputs.size() != 1)
    {
      throw fetch::math::exceptions::WrongShape(
          "Can not do a backward pass through Dropout layer: wrong inputs provided.");
    }
    if (error_signal.shape() != inputs.front()->shape())
    {
      throw fetch::math::exceptions::WrongShape(
          "Can not do a backward pass through Dropout layer: error signal shape mismatch inputs "
          "shape.");
    }
    if (scaling_coefficients_.shape() != inputs.front()->shape())
    {
      throw fetch::math::exceptions::WrongShape(
          "Can not do a backward pass through Dropout layer: dropout scaling coefficients shape "
          "mismatch inputs shape.");
    }

    TensorType return_signal{error_signal.shape()};

    // gradient of dropout is 1.0/keep_prob for enabled neurons and 0.0 for disabled
    // multiply by error_signal (chain rule)

    fetch::math::Multiply(error_signal, scaling_coefficients_, return_signal);

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_DROPOUT;
  }
  static constexpr char const *DESCRIPTOR = "Dropout";

private:
  TensorType scaling_coefficients_;
  DataType   probability_of_keeping_{1};
  RNG        rng_;

  DataType const zero_{0};
  DataType const one_{1};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
