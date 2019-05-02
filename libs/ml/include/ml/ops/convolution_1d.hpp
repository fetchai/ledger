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
class Convolution1D : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Convolution1D(SizeType stride_size = 1)
    : stride_size_(stride_size)
  {}

  ~Convolution1D() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 2);
    // Input should be a 2D tensor [C x H]
    ASSERT(inputs.at(0).get().shape().size() == 2);
    // Kernels should be a 3D tensor [oC x iC x H]
    ASSERT(inputs.at(1).get().shape().size() == 3);

    auto output_shape = ComputeOutputShape(inputs);
    ASSERT(output.shape() == output_shape);

    ArrayType input   = inputs.at(0).get();
    ArrayType kernels = inputs.at(1).get();

    SizeType input_channels  = input.shape().at(0);
    SizeType output_channels = kernels.shape().at(0);
    SizeType kernel_height   = kernels.shape().at(2);
    SizeType output_height   = output_shape.at(1);

    SizeType horizontal_stride_width  = kernel_height * input_channels;
    SizeType horizontal_stride_height = output_height;
    SizeType vertical_stride_width    = output_channels;

    ArrayType horizontal_stride({horizontal_stride_width, horizontal_stride_height});
    ArrayType vertical_stride({vertical_stride_width, horizontal_stride_width});

    // Fill horizontal stride
    FillHorizontalStride(input, horizontal_stride, output_height, input_channels, kernel_height);

    // Fill vertical stride
    FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height);

    // Do matmul
    ArrayType reshaped_output = fetch::math::Dot(vertical_stride, horizontal_stride);

    // Put values to output
    FillOutput(reshaped_output, output, output_channels, output_height);

    return output;
  }

  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    ASSERT(inputs.size() == 2);
    // Input should be a 2D tensor [C x H]
    ASSERT(inputs.at(0).get().shape().size() == 2);
    // Kernels should be a 3D tensor [oC x iC x H]
    ASSERT(inputs.at(1).get().shape().size() == 3);

    auto output_shape = ComputeOutputShape(inputs);
    ASSERT(error_signal.shape() == output_shape);

    SizeType output_height = output_shape.at(1);

    ArrayType input   = inputs.at(0).get();
    ArrayType kernels = inputs.at(1).get();

    SizeType  input_channels  = input.shape().at(0);
    SizeType  output_channels = kernels.shape().at(0);
    SizeType  kernel_height   = kernels.shape().at(2);
    ArrayType error_signal1(input.shape());
    ArrayType error_signal2(kernels.shape());

    SizeType horizontal_stride_width  = kernel_height * input_channels;
    SizeType horizontal_stride_height = output_height;
    SizeType vertical_stride_width    = output_channels;

    ArrayType horizontal_stride({horizontal_stride_width, horizontal_stride_height});
    ArrayType vertical_stride({vertical_stride_width, horizontal_stride_width});

    // Fill horizontal stride
    FillHorizontalStride(input, horizontal_stride, output_height, input_channels, kernel_height);

    // Fill vertical stride
    FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height);

    ArrayType tmp_error{{vertical_stride_width, horizontal_stride_height}};

    // Reverse put values to output
    ReverseFillOutput(tmp_error, error_signal, output_channels, output_height);

    // Backwards matmul
    ArrayType error2 = fetch::math::DotTranspose(tmp_error, horizontal_stride);
    ArrayType error1 = fetch::math::TransposeDot(vertical_stride, tmp_error);

    // Reverse fill horizontal stride
    ReverseFillHorizontalStride(error_signal1, error1, output_height, input_channels,
                                kernel_height);

    // Reverse fill vertical stride
    ReverseFillVerticalStride(error_signal2, error2, output_channels, input_channels,
                              kernel_height);

    return {error_signal1, error_signal2};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    if (output_shape_.size() != 0)
      return output_shape_;
    output_shape_.emplace_back(inputs.at(1).get().shape()[0]);
    output_shape_.emplace_back(
        (inputs.at(0).get().shape()[1] - inputs.at(1).get().shape()[2] + stride_size_) /
        stride_size_);
    return output_shape_;
  }

  static constexpr char const *DESCRIPTOR = "Convolution1D";

private:
  void FillVerticalStride(ArrayType const &input, ArrayType &vertical_stride,
                          SizeType const output_channels, SizeType const input_channels,
                          SizeType const kernel_height)
  {
    SizeType j_s;                                           // stride height iterator
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
    {
      j_s = 0;
      for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          vertical_stride.Set(i_oc, j_s, input.At(i_oc, i_ic, i_k));
          ++j_s;
        }
      }
    }
  }

  void ReverseFillVerticalStride(ArrayType &input, ArrayType const &vertical_stride,
                                 SizeType const output_channels, SizeType const input_channels,
                                 SizeType const kernel_height)
  {
    SizeType j_s;                                           // stride height iterator
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
    {
      j_s = 0;
      for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          input.Set(i_oc, i_ic, i_k, vertical_stride.At(i_oc, j_s));
          ++j_s;
        }
      }
    }
  }

  void FillHorizontalStride(ArrayType const &input, ArrayType &horizontal_stride,
                            SizeType const output_height, SizeType const input_channels,
                            SizeType const kernel_height)
  {
    SizeType i_s;  // stride width iterator
    SizeType j_s;  // stride height iterator

    j_s = 0;
    for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
    {

      i_s = 0;
      for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          horizontal_stride.Set(i_s, j_s, input.At(i_ic, i_o * stride_size_ + i_k));
          ++i_s;
        }
      }
      ++j_s;
    }
  }

  void ReverseFillHorizontalStride(ArrayType &input, ArrayType const &horizontal_stride,
                                   SizeType const output_height, SizeType const input_channels,
                                   SizeType const kernel_height)
  {
    SizeType i_s;  // stride width iterator
    SizeType j_s;  // stride height iterator

    j_s = 0;
    for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
    {

      i_s = 0;
      for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          input.Set(i_ic, i_o * stride_size_ + i_k, horizontal_stride.At(i_s, j_s));
          ++i_s;
        }
      }
      ++j_s;
    }
  }

  void FillOutput(ArrayType const &gemm_output, ArrayType &output, SizeType const output_channels,
                  SizeType const output_height)
  {
    SizeType i_tmp;
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
    {
      i_tmp = 0;
      for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
      {

        output.Set(i_oc, i_o, gemm_output.At(i_oc, i_tmp));
        ++i_tmp;
      }
    }
  }

  void ReverseFillOutput(ArrayType &gemm_output, ArrayType const &output,
                         SizeType const output_channels, SizeType const output_height)
  {
    SizeType i_tmp;
    for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
    {
      i_tmp = 0;
      for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
      {

        gemm_output.Set(i_oc, i_tmp, output.At(i_oc, i_o));
        ++i_tmp;
      }
    }
  }

  SizeType              stride_size_;
  std::vector<SizeType> output_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
