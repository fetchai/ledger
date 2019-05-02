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
class MaxPool1D : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MaxPool1D(SizeType const kernel_size, SizeType const stride_size)
    : kernel_size_{kernel_size}
    , stride_size_{stride_size}
  {}

  ~MaxPool1D() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 1);
    // Input should be a 2D tensor [C x W]
    ASSERT(inputs.at(0).get().shape().size() == 2);

    auto outputShape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == outputShape);
    SizeType iter;
    DataType max;
    DataType val;
    for (SizeType c{0}; c < outputShape[0]; ++c)  // Iterate over output channels
    {

      for (SizeType i{0}; i < outputShape.at(1); i++)  // Iterate over kernel stride
      {
        iter = i * stride_size_;

        max = inputs.at(0).get().At(c, iter);

        for (SizeType j{0}; j < kernel_size_; j++)  // Iterate over kernel width
        {
          val = inputs.at(0).get().At(c, iter + j);
          if (val > max)
          {
            max = val;
          }
        }
        output.Set(c, i, max);
      }
    }
    return output;
  }

  // Gradient of max pool is passed only to max node
  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(error_signal.shape() == ComputeOutputShape(inputs));

    ArrayType returnSignal{inputs.at(0).get().shape()};

    auto outputShape = ComputeOutputShape(inputs);

    SizeType iter;
    DataType max;
    DataType val;
    SizeType max_iter;
    for (SizeType c{0}; c < outputShape[0]; ++c)  // Iterate over output channels
    {
      for (SizeType i{0}; i * stride_size_ < inputs.at(0).get().shape()[1] - kernel_size_;
           i++)  // Iterate over kernel stride
      {
        iter     = i * stride_size_;
        max      = inputs.at(0).get().At(c, iter);
        max_iter = iter;

        for (SizeType j{0}; j < kernel_size_; j++)  // Iterate over kernel width
        {
          val = inputs.at(0).get().At(c, iter + j);
          if (val > max)
          {
            max      = val;
            max_iter = iter + j;
          }
        }
        // Error needs to be added if same node occurs in multiple output nodes at once
        returnSignal.Set(c, max_iter, returnSignal.At(c, max_iter) + error_signal.At(c, i));
      }
    }

    return {returnSignal};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    // Return pre-computed value if exist
    if (output_shape_.size() != 0)
      return output_shape_;
    // output_shape_[0]=number of output channels
    output_shape_.emplace_back(inputs.at(0).get().shape().at(0));
    // output_shape_[1]=number of stride_size steps on input size
    output_shape_.emplace_back((inputs.at(0).get().shape().at(1) - (kernel_size_ - stride_size_)) /
                               stride_size_);
    return output_shape_;
  }

  static constexpr char const *DESCRIPTOR = "MaxPool1D";

private:
  SizeType              kernel_size_;
  SizeType              stride_size_;
  std::vector<SizeType> output_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
