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

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class BatchwiseAddOp : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using VecTensorType = typename ElementWiseOps<T>::VecTensorType;
  using SizeType      = typename ArrayType::SizeType;

  BatchwiseAddOp()          = default;
  virtual ~BatchwiseAddOp() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().at(0) == inputs.at(1).get().shape().at(0));
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::BatchwiseAdd(inputs[0].get(), inputs[1].get(), output);
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().at(0) == inputs.at(1).get().shape().at(0));
    assert(error_signal.size() == inputs.at(0).get().size());

    SizeType batch_dimension = inputs.front().get().shape().size() - 1;
    return {error_signal, fetch::math::ReduceSum(error_signal, batch_dimension)};
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Add";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
