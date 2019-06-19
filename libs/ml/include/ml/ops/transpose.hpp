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

#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Transpose : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename BatchOps<T>::VecTensorType;

  Transpose()          = default;
  virtual ~Transpose() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(output.shape() == this->ComputeOutputShape(inputs));

    if (output.shape().size() == 2)
    {
      output.Copy(inputs.front().get().Transpose());
    }
    else
    {
      output.Copy(inputs.front().get().Transpose(transpose_vector_));
    }
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(error_signal.shape() == this->ComputeOutputShape(inputs));

    if (error_signal.shape().size() == 2)
    {
      return {error_signal.Transpose()};
    }
    else
    {
      return {error_signal.Transpose(transpose_vector_)};
    }
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    // Normal transpose
    if (inputs.at(0).get().shape().size() == 2)
    {
      return {inputs.front().get().shape().at(1), inputs.front().get().shape().at(0)};
    }
    // Batchwise transpose
    else
    {
      return {inputs.front().get().shape().at(1), inputs.front().get().shape().at(0),
              inputs.at(0).get().shape().at(2)};
    }
  }

  std::vector<SizeType>        transpose_vector_ = {1, 0, 2};
  static constexpr char const *DESCRIPTOR        = "Transpose";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
