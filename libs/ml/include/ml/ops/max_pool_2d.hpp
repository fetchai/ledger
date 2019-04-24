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
class MaxPool2d : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MaxPool2d(SizeType kernel_size, SizeType stride_size)
    : kernel_size_(kernel_size)
    , stride_size_(stride_size)
  {}

  virtual ~MaxPool2d() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 1);
    // Input should be a 3D tensor [C x W x H]
    ASSERT(inputs.at(0).get().shape().size() == 3);

    auto outputShape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == outputShape);
    for (uint64_t c(0); c < outputShape[0]; ++c)  // Iterate over output channels
    {

      for (uint64_t iw(0); iw < outputShape.at(1); iw++)  // Iterate width over kernel stride
      {
        SizeType iterw = iw * stride_size_;

        for (uint64_t ih(0); ih < outputShape.at(2); ih++)  // Iterate height over kernel stride
        {
          SizeType iterh = ih * stride_size_;

          DataType max(inputs.at(0).get().At(c, iterw, iterh));

          for (uint64_t jw(0); jw < kernel_size_; jw++)  // Iterate over kernel width
          {
            for (uint64_t jh(0); jh < kernel_size_; jh++)  // Iterate over kernel width
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
  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(errorSignal.shape() == inputs.front().get().shape());

    ArrayType returnSignal{errorSignal.shape()};

    auto outputShape = ComputeOutputShape(inputs);

    for (uint64_t c(0); c < outputShape[0]; ++c)  // Iterate over output channels
    {

      for (uint64_t iw(0); iw < outputShape.at(1); iw++)  // Iterate width over kernel stride
      {
        SizeType iterw = iw * stride_size_;

        for (uint64_t ih(0); ih < outputShape.at(2); ih++)  // Iterate height over kernel stride
        {
          SizeType iterh = ih * stride_size_;

          DataType max(inputs.at(0).get().At(c, iterw, iterh));
          SizeType max_iterw(iterw);
          SizeType max_iterh(iterh);

          for (uint64_t jw(0); jw < kernel_size_; jw++)  // Iterate over kernel width
          {
            for (uint64_t jh(0); jh < kernel_size_; jh++)  // Iterate over kernel width
            {

              DataType val = inputs.at(0).get().At(c, iterw + jw, iterh + jh);
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
                           returnSignal.At(c, max_iterw, max_iterh) + DataType(1));
        }
      }
    }

    // multiply by errorSignal (chain rule)
    fetch::math::Multiply(errorSignal, returnSignal, returnSignal);

    return {returnSignal};
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    std::vector<typename ArrayType::SizeType> outputShape;
    outputShape.push_back(inputs.at(0).get().shape().at(0));
    outputShape.push_back((inputs.at(0).get().shape().at(1) - (kernel_size_ - stride_size_)) /
                          stride_size_);
    outputShape.push_back((inputs.at(0).get().shape().at(2) - (kernel_size_ - stride_size_)) /
                          stride_size_);

    return outputShape;
  }

  static constexpr char const *DESCRIPTOR = "MaxPool2d";

private:
  SizeType kernel_size_;
  SizeType stride_size_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
