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

#include <cassert>
#include <memory>
#include <vector>

#include "math/ml/loss_functions/mean_square_error.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MeanSquareError : public Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  MeanSquareError()          = default;
  virtual ~MeanSquareError() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].get().shape() == inputs[1].get().shape());

    SizeType batch_dim = inputs.at(0).get().shape().size() - 1;
    for (SizeType i{0}; i < inputs.at(0).get().shape().at(batch_dim); i++)
    {
      typename ArrayType::Type ret = fetch::math::MeanSquareError(
          inputs[0].get().Slice(i, batch_dim), inputs[1].get().Slice(i, batch_dim));
      output(0, i) = ret;
    }
  }

  // grad[0]=2*err*(in[0]-in[1])/mean_size, grad[1]=-2*err*(in[0]-in[1])/mean_size,
  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].get().shape() == inputs[1].get().shape());

    ArrayType return_signal1(inputs.front().get().shape());
    ArrayType return_signal2(inputs.front().get().shape());

    // return_signal=in[0]-in[1]
    fetch::math::Subtract(inputs.at(0).get(), inputs.at(1).get(), return_signal1);

    // return_signal=err*(in[0]-in[1])
    // I am not sure about that
    (void)error_signal;
    // fetch::math::Multiply(return_signal1, error_signal, return_signal1);

    // return_signal=(2*err*(in[0]-in[1]))/mean_size
    DataType mean_size = static_cast<DataType>(inputs.at(0).get().shape().at(0));
    fetch::math::Multiply(return_signal1, DataType{2} / mean_size, return_signal1);

    fetch::math::Multiply(return_signal1, DataType{-1}, return_signal2);
    return {return_signal1, return_signal2};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return {1, inputs.at(0).get().shape().at(inputs.at(0).get().shape().size() - 1)};
  }

  static constexpr char const *DESCRIPTOR = "MeanSquareError";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
