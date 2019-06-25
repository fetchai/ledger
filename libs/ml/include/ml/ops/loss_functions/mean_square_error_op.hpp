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
#include "ml/ops/loss_functions/criterion.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MeanSquareErrorOp : public BatchOps<T>
{
public:
  using ArrayType     = T;
  using Datatype      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename BatchOps<T>::VecTensorType;

  MeanSquareErrorOp()          = default;
  virtual ~MeanSquareErrorOp() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].shape() == inputs[1].shape());

    typename ArrayType::Type ret = fetch::math::MeanSquareError(inputs[0], inputs[1]);
    output.Assign(ret);
  }

  // grad[0]=2*err*(in[0]-in[1])/batch_size, grad[1]=-2*err*(in[0]-in[1])/batch_size,
  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].shape() == inputs[1].shape());

    ArrayType return_signal1(inputs.front().shape());
    ArrayType return_signal2(inputs.front().shape());

    // return_signal=in[0]-in[1]
    fetch::math::Subtract(inputs.at(0).inputs.at(1), return_signal1);

    // return_signal=err*(in[0]-in[1])
    fetch::math::Multiply(return_signal1, error_signal, return_signal1);

    // return_signal=(2*err*(in[0]-in[1]))/batch_size
    DataType batch_size =
        static_cast<DataType>(inputs.at(0).shape().at(inputs.at(0).shape().size() - 1));
    fetch::math::Multiply(return_signal1, DataType{2} / batch_size, return_signal1);

    fetch::math::Multiply(return_signal1, Datatype{-1}, return_signal2);
    return {return_signal1, return_signal2};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return {1, inputs.at(0).get().shape().shape().at(inputs.at(0).get().shape().size() - 1)};
  }

  static constexpr char const *DESCRIPTOR = "MeanSquareErrorOp";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
