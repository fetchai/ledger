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
class MaxPool2D : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MaxPool2D(SizeType const kernel_size, SizeType const stride_size)
    : kernel_size_{kernel_size}
    , stride_size_{stride_size}
  {}

  ~MaxPool2D() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 1);
    // Input should be a 3D tensor [C x W x H]
    ASSERT(inputs.at(0).get().shape().size() == 3);

    auto outputShape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == outputShape);
    for (SizeType c{0}; c < outputShape[0]; ++c)  // Iterate over output channels
    {

      for (SizeType iw{0}; iw < outputShape.at(1); iw++)  // Iterate width over kernel stride
      {
        SizeType iterw = iw * stride_size_;

        for (SizeType ih{0}; ih < outputShape.at(2); ih++)  // Iterate height over kernel stride
        {
          SizeType iterh = ih * stride_size_;

          DataType max(inputs.at(0).get().At(c, iterw, iterh));

          for (SizeType jw{0}; jw < kernel_size_; jw++)  // Iterate over kernel width
          {
            for (SizeType jh{0}; jh < kernel_size_; jh++)  // Iterate over kernel width
            {

              DataType val = inputs.at(0).get().At(c, iterw + jw, iterh + jh);
              if (val > max)
              {
                max = val;
              }
            }
          }

          output.Set(c, iw, ih, max);
        }
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

    SizeType iterh;
    SizeType iterw;
    DataType max;
    DataType val;
    SizeType max_iterw;
    SizeType max_iterh;
    for (SizeType c{0}; c < outputShape[0]; ++c)  // Iterate over output channels
    {

      for (SizeType iw{0}; iw < outputShape.at(1); iw++)  // Iterate width over kernel stride
      {
        iterw = iw * stride_size_;
        for (SizeType ih{0}; ih < outputShape.at(2); ih++)  // Iterate height over kernel stride
        {
          iterh     = ih * stride_size_;
          max       = inputs.at(0).get().At(c, iterw, iterh);
          max_iterw = iterw;
          max_iterh = iterh;

          for (SizeType jw{0}; jw < kernel_size_; jw++)  // Iterate over kernel width
          {
            for (SizeType jh{0}; jh < kernel_size_; jh++)  // Iterate over kernel width
            {

              val = inputs.at(0).get().At(c, iterw + jw, iterh + jh);
              if (val > max)
              {
                max       = val;
                max_iterw = iterw + jw;
                max_iterh = iterh + jh;
              }
            }
          }

          // Error needs to be added if same node occurs in multiple output nodes at once
          returnSignal.Set(c, max_iterw, max_iterh,
                           returnSignal.At(c, max_iterw, max_iterh) + error_signal.At(c, iw, ih));
        }
      }
    }

    return {returnSignal};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    if (output_shape_.size() != 0)
      return output_shape_;
    output_shape_.emplace_back(inputs.at(0).get().shape().at(0));
    output_shape_.emplace_back((inputs.at(0).get().shape().at(1) - (kernel_size_ - stride_size_)) /
                               stride_size_);
    output_shape_.emplace_back((inputs.at(0).get().shape().at(2) - (kernel_size_ - stride_size_)) /
                               stride_size_);
    return output_shape_;
  }

  static constexpr char const *DESCRIPTOR = "MaxPool2D";

private:
  SizeType              kernel_size_;
  SizeType              stride_size_;
  std::vector<SizeType> output_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
