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

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Transpose : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = TransposeSaveableParams;

  explicit Transpose(std::vector<SizeType> transpose_vector = {1, 0, 2})
    : transpose_vector_(transpose_vector)
  {}

  explicit Transpose(SPType const &sp)
  {
    transpose_vector_ = sp.transpose_vector;
  }

  ~Transpose() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams()
  {
    SPType sp{};
    sp.DESCRIPTOR       = DESCRIPTOR;
    sp.transpose_vector = transpose_vector_;
    return std::make_shared<SPType>(sp);
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    if (inputs.front().get().shape().size() == 2)
    {
      output.Copy(inputs.front().get().Transpose());
    }
    else
    {
      output.Copy(inputs.front().get().Transpose(transpose_vector_));
    }
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    if (error_signal.shape().size() == 2)
    {
      return {error_signal.Transpose()};
    }
    else
    {
      return {error_signal.Transpose(transpose_vector_)};
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    // 2D transpose
    if (inputs.at(0).get().shape().size() == 2)
    {
      return {inputs.front().get().shape().at(1), inputs.front().get().shape().at(0)};
    }
    // Transpose by given vector
    else
    {
      std::vector<SizeType> input_shape = inputs.front().get().shape();
      std::vector<SizeType> shape;

      shape.reserve(shape.size());
      for (auto &current_size : transpose_vector_)
      {
        shape.push_back(input_shape.at(current_size));
      }

      return shape;
    }
  }

  std::vector<SizeType>        transpose_vector_;
  static constexpr char const *DESCRIPTOR = "Transpose";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
