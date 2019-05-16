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
class Convolution2D : public BatchOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Convolution2D(SizeType stride_size = 1)
    : stride_size_(stride_size)
  {}

  ~Convolution2D() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output) override;

  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           error_signal_signal) override;

  std::vector<typename ArrayType::SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const override;

  static constexpr char const *DESCRIPTOR = "Convolution2D";

private:
  void FillVerticalStride(ArrayType &input, ArrayType &vertical_stride,
                          SizeType const output_channels, SizeType const input_channels,
                          SizeType const kernel_height, SizeType const kernel_width);

  void ReverseFillVerticalStride(ArrayType &input, ArrayType &vertical_stride,
                                 SizeType const output_channels, SizeType const input_channels,
                                 SizeType const kernel_height, SizeType const kernel_width);

  void FillHorizontalStride(ArrayType &input, ArrayType &horizontal_stride,
                            SizeType const output_height, SizeType const output_width,
                            SizeType const input_channels, SizeType const kernel_height,
                            SizeType const kernel_width);

  void ReverseFillHorizontalStride(ArrayType &input, ArrayType &horizontal_stride,
                                   SizeType const output_height, SizeType const output_width,
                                   SizeType const input_channels, SizeType const kernel_height,
                                   SizeType const kernel_width);

  void FillOutput(ArrayType const &gemm_output, ArrayType &output, SizeType const output_channels,
                  SizeType const output_height, SizeType const output_width);

  void ReverseFillOutput(ArrayType &gemm_output, ArrayType const &output,
                         SizeType const output_channels, SizeType const output_height,
                         SizeType const output_width);

  SizeType stride_size_;
};

/**
 * Applies 2D convolution using im2col with General Matrix Multiplication described here:
 * https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height x input_width], inputs[1] =
 * kernel_data[kernel_channels x kernel_height x kernel_width]
 * @param output tensor of size [output_channels x number_of_stride_sized_steps_over_input_height x
 * number_of_stride_sized_steps_over_input_width]
 * @return: output tensor parameter
 */
template <class ArrayType>
ArrayType Convolution2D<ArrayType>::Forward(
    std::vector<std::reference_wrapper<ArrayType const>> const &inputs, ArrayType &output)
{
  ASSERT(inputs.size() == 2);
  // Input should be a 3D tensor [C x H x W]
  ASSERT(inputs.at(0).get().shape().size() == 3);
  // Kernels should be a 4D tensor [oC x iC x H x W]
  ASSERT(inputs.at(1).get().shape().size() == 4);
  ASSERT(output.shape() == ComputeOutputShape(inputs));

  ArrayType input   = inputs.at(0).get();
  ArrayType kernels = inputs.at(1).get();

  SizeType input_channels  = input.shape().at(0);
  SizeType output_channels = kernels.shape().at(0);
  SizeType kernel_height   = kernels.shape().at(2);
  SizeType kernel_width    = kernels.shape().at(3);
  SizeType output_height   = output.shape().at(1);
  SizeType output_width    = output.shape().at(2);

  SizeType horizontal_stride_width  = kernel_width * kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * output_width;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  ArrayType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  ArrayType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, output_width, input_channels,
                       kernel_height, kernel_width);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height,
                     kernel_width);

  // Do matmul
  ArrayType reshaped_output = fetch::math::Dot(vertical_stride, horizontal_stride);

  // Reshape values after matmul to output
  FillOutput(reshaped_output, output, output_channels, output_height, output_width);

  return output;
}

/**
 * Computes gradient of 2D convolution using reversed im2col and General Matrix Multiplication
 * described here: https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height x input_width], inputs[1] =
 * kernel_data[kernel_channels x kernel_height x kernel_width]
 * @param error_signal tensor of size [output_channels x
 * number_of_stride_sized_steps_over_input_height x number_of_stride_sized_steps_over_input_width]
 * @return: output vector of tensors with back propagated error signal
 * output[0]=input_error[inputs[0].shape], output[1]=kernel_error[inputs[1].shape]
 */
template <class ArrayType>
std::vector<ArrayType> Convolution2D<ArrayType>::Backward(
    std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
    ArrayType const &                                           error_signal_signal)
{
  ASSERT(inputs.size() == 2);
  // Input should be a 3D tensor [C x H x W]
  ASSERT(inputs.at(0).get().shape().size() == 3);
  // Kernels should be a 4D tensor [oC x iC x H x W]
  ASSERT(inputs.at(1).get().shape().size() == 4);
  ASSERT(error_signal_signal.shape() == ComputeOutputShape(inputs));

  SizeType output_height = error_signal_signal.shape().at(1);
  SizeType output_width  = error_signal_signal.shape().at(2);

  ArrayType input   = inputs.at(0).get();
  ArrayType kernels = inputs.at(1).get();

  SizeType  input_channels  = input.shape().at(0);
  SizeType  output_channels = kernels.shape().at(0);
  SizeType  kernel_height   = kernels.shape().at(2);
  SizeType  kernel_width    = kernels.shape().at(3);
  ArrayType input_error(input.shape());
  ArrayType kernel_error(kernels.shape());

  SizeType horizontal_stride_width  = kernel_width * kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * output_width;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  ArrayType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  ArrayType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, output_width, input_channels,
                       kernel_height, kernel_width);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height,
                     kernel_width);

  // Reshape error_signal to error for matmul
  ArrayType error{{vertical_stride_width, horizontal_stride_height}};
  ReverseFillOutput(error, error_signal_signal, output_channels, output_height, output_width);

  // Backwards matmul
  ArrayType error2 = fetch::math::DotTranspose(error, horizontal_stride);
  ArrayType error1 = fetch::math::TransposeDot(vertical_stride, error);

  // Reshape horizontal stride to input data error_signal - reversed im2col
  ReverseFillHorizontalStride(input_error, error1, output_height, output_width, input_channels,
                              kernel_height, kernel_width);

  // Reshape vertical stride to kernel data error_signal - reversed im2col
  ReverseFillVerticalStride(kernel_error, error2, output_channels, input_channels, kernel_height,
                            kernel_width);

  return {input_error, kernel_error};
}

template <class ArrayType>
std::vector<typename ArrayType::SizeType> Convolution2D<ArrayType>::ComputeOutputShape(
    std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
{
  std::vector<SizeType> output_shape;

  // output_shape_[0]=number of output channels
  output_shape.emplace_back(inputs.at(1).get().shape()[0]);
  // output_shape_[1]=number of stride_size steps over input height
  output_shape.emplace_back(
      (inputs.at(0).get().shape()[1] - inputs.at(1).get().shape()[2] + stride_size_) /
      stride_size_);
  // output_shape_[2]=number of stride_size steps over input width
  output_shape.emplace_back(
      (inputs.at(0).get().shape()[2] - inputs.at(1).get().shape()[3] + stride_size_) /
      stride_size_);
  return output_shape;
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes input tensor to vertical_stride tensor using im2col
 * @tparam ArrayType
 * @param input
 * @param vertical_stride
 * @param output_channels
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class ArrayType>
void Convolution2D<ArrayType>::FillVerticalStride(ArrayType &input, ArrayType &vertical_stride,
                                                  SizeType const output_channels,
                                                  SizeType const input_channels,
                                                  SizeType const kernel_height,
                                                  SizeType const kernel_width)
{
  SizeType j_s;                                           // stride height iterator
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    j_s = 0;
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
    {

      for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
      {
        for (SizeType j_k(0); j_k < kernel_width; j_k++)  // Iterate over kernel width
        {
          vertical_stride.Set(i_oc, j_s, input.At(i_oc, i_ic, i_k, j_k));
          ++j_s;
        }
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes vertical_stride tensor to input tensor using reversed im2col
 * @tparam ArrayType
 * @param input
 * @param vertical_stride
 * @param output_channels
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class ArrayType>
void Convolution2D<ArrayType>::ReverseFillVerticalStride(
    ArrayType &input, ArrayType &vertical_stride, SizeType const output_channels,
    SizeType const input_channels, SizeType const kernel_height, SizeType const kernel_width)
{
  SizeType j_s;                                           // stride height iterator
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    j_s = 0;
    for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
    {

      for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
      {
        for (SizeType j_k(0); j_k < kernel_width; j_k++)  // Iterate over kernel width
        {
          input.Set(i_oc, i_ic, i_k, j_k, vertical_stride.At(i_oc, j_s));
          ++j_s;
        }
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes kernel(input) tensor to horizontal_stride tensor using im2col
 * @tparam ArrayType
 * @param input
 * @param horizontal_stride
 * @param output_height
 * @param output_width
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class ArrayType>
void Convolution2D<ArrayType>::FillHorizontalStride(ArrayType &input, ArrayType &horizontal_stride,
                                                    SizeType const output_height,
                                                    SizeType const output_width,
                                                    SizeType const input_channels,
                                                    SizeType const kernel_height,
                                                    SizeType const kernel_width)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index

  j_s = 0;
  for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
  {
    for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
    {
      i_s = 0;
      for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          for (SizeType j_k(0); j_k < kernel_width; j_k++)  // Iterate over kernel width
          {
            horizontal_stride.Set(
                i_s, j_s, input.At(i_ic, i_o * stride_size_ + i_k, j_o * stride_size_ + j_k));
            ++i_s;
          }
        }
      }
      ++j_s;
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes horizontal_stride tensor to kernel(input) tensor using reversed im2col
 * @tparam ArrayType
 * @param input
 * @param horizontal_stride
 * @param output_height
 * @param output_width
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class ArrayType>
void Convolution2D<ArrayType>::ReverseFillHorizontalStride(
    ArrayType &input, ArrayType &horizontal_stride, SizeType const output_height,
    SizeType const output_width, SizeType const input_channels, SizeType const kernel_height,
    SizeType const kernel_width)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index

  j_s = 0;
  for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
  {
    for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
    {
      i_s = 0;
      for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          for (SizeType j_k(0); j_k < kernel_width; j_k++)  // Iterate over kernel width
          {
            input.Set(i_ic, i_o * stride_size_ + i_k, j_o * stride_size_ + j_k,
                      horizontal_stride.At(i_s, j_s));
            ++i_s;
          }
        }
      }
      ++j_s;
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshape gemm_output tensor (result of matmul on vertical and horizontal stride) to output tensor
 * @tparam ArrayType
 * @param gemm_output
 * @param output
 * @param output_channels
 * @param output_height
 * @param output_width
 */
template <class ArrayType>
void Convolution2D<ArrayType>::FillOutput(ArrayType const &gemm_output, ArrayType &output,
                                          SizeType const output_channels,
                                          SizeType const output_height, SizeType const output_width)
{

  SizeType it;
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    it = 0;
    for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
      {
        output.Set(i_oc, i_o, j_o, gemm_output.At(i_oc, it));
        ++it;
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshape output tensor to gemm_output tensor (result of matmul on vertical and horizontal stride)
 * @tparam ArrayType
 * @param gemm_output
 * @param output
 * @param output_channels
 * @param output_height
 * @param output_width
 */
template <class ArrayType>
void Convolution2D<ArrayType>::ReverseFillOutput(ArrayType &gemm_output, ArrayType const &output,
                                                 SizeType const output_channels,
                                                 SizeType const output_height,
                                                 SizeType const output_width)
{

  SizeType it;
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    it = 0;
    for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
    {
      for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
      {
        gemm_output.Set(i_oc, it, output.At(i_oc, i_o, j_o));
        ++it;
      }
    }
  }
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
