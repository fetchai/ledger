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

#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LayerNorm : public Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  explicit LayerNorm(SizeType axis    = static_cast<SizeType>(0),
                     DataType epsilon = fetch::math::function_tolerance<DataType>())
    : epsilon_(epsilon)
    , axis_(axis)
  {}
  ~LayerNorm() override = default;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.at(0)->shape();
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    // cache this inputs
    prev_input_ = *(inputs.front());

    // the layernorm can only be applied on the first axis
    assert(inputs.size() == 1);
    assert(output.shape() == inputs.front()->shape());

    // do normalization along axis = 0
    // recenter input
    ArrayType mu = fetch::math::ReduceMean(*(inputs.front()), axis_);
    fetch::math::Subtract(*(inputs.front()), mu, mu);

    // get variance of input
    ArrayType sq_mu = fetch::math::Square(mu);
    ArrayType var   = fetch::math::ReduceMean(sq_mu, axis_);
    fetch::math::Add(var, epsilon_, var);

    // normalize input
    fetch::math::Sqrt(var, var);
    fetch::math::Divide(mu, var, mu);
    fetch::math::Divide(static_cast<DataType>(1), var, var);

    // cache data for backward part
    cached_inv_sqrt_var_ = var;
    cached_output_       = mu;

    output = cached_output_;
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    // plz refer to https://kevinzakka.github.io/2016/09/14/batch_normalization/ for detailed
    // derivation of gradient N.B. gradient for batch norm and layer norm is the same, apart from
    // the change of axis.

    // make sure we have run forward for this inputs
    if (prev_input_ != *(inputs.front()))
    {
      // if this is a new input, run the forward again
      cached_output_.Reshape(inputs.front()->shape());
      Forward(inputs, cached_output_);
    }

    // do the backward
    ArrayType output_error_signal;
    auto      feature_length = static_cast<DataType>(inputs.front()->shape(axis_));
    auto      backward_a     = fetch::math::Multiply(error_signal, feature_length);
    auto      backward_b     = fetch::math::ReduceSum(error_signal, axis_);
    auto      backward_c =
        cached_output_ *
        fetch::math::ReduceSum(fetch::math::Multiply(error_signal, cached_output_), axis_);
    output_error_signal =
        cached_inv_sqrt_var_ / feature_length * (backward_a - backward_b - backward_c);

    return {output_error_signal};
  }

  static constexpr char const *DESCRIPTOR = "LayerNormalization";

private:
  DataType epsilon_;
  SizeType axis_;

  ArrayType prev_input_;
  ArrayType cached_inv_sqrt_var_;
  ArrayType cached_output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
