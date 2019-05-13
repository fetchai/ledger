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
                    ArrayType &                                                 output) override;

  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           error_signal) override;

  std::vector<typename ArrayType::SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) override;

  static constexpr char const *DESCRIPTOR = "Convolution1D";

private:
  void FillVerticalStride(ArrayType const &input, ArrayType &vertical_stride,
                          SizeType const output_channels, SizeType const input_channels,
                          SizeType const kernel_height);

  void ReverseFillVerticalStride(ArrayType &input, ArrayType const &vertical_stride,
                                 SizeType const output_channels, SizeType const input_channels,
                                 SizeType const kernel_height);

  void FillHorizontalStride(ArrayType const &input, ArrayType &horizontal_stride,
                            SizeType const output_height, SizeType const input_channels,
                            SizeType const kernel_height);

  void ReverseFillHorizontalStride(ArrayType &input, ArrayType const &horizontal_stride,
                                   SizeType const output_height, SizeType const input_channels,
                                   SizeType const kernel_height);

  void FillOutput(ArrayType const &gemm_output, ArrayType &output, SizeType const output_channels,
                  SizeType const output_height);

  void ReverseFillOutput(ArrayType &gemm_output, ArrayType const &output,
                         SizeType const output_channels, SizeType const output_height);

  SizeType              stride_size_;
  std::vector<SizeType> output_shape_;
};

/**
 * Applies 1D convolution using im2col with General Matrix Multiplication described here:
 * https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height], inputs[1] = kernel_data[kernel_channels x
 * kernel_height]
 * @param output tensor of size [output_channels x number_of_stride_sized_steps]
 * @return: output tensor parameter
 */
template <class ArrayType>
ArrayType Convolution1D<ArrayType>::Forward(
    std::vector<std::reference_wrapper<ArrayType const>> const &inputs, ArrayType &output)
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

  // Horizontal stride contains input data
  ArrayType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  ArrayType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, input_channels, kernel_height);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height);

  // Do matmul
  ArrayType reshaped_output = fetch::math::Dot(vertical_stride, horizontal_stride);

  // Reshape values after matmul to output
  FillOutput(reshaped_output, output, output_channels, output_height);

  return output;
}

/**
 * Computes gradient of 1D convolution using reversed im2col and General Matrix Multiplication
 * described here: https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height], inputs[1] = kernel_data[kernel_channels x
 * kernel_height]
 * @param error_signal tensor of size [output_channels x number_of_stride_sized_steps]
 * @return: output vector of tensors with back propagated error signal
 * output[0]=input_error[inputs[0].shape], output[1]=kernel_error[inputs[1].shape]
 */
template <class ArrayType>
std::vector<ArrayType> Convolution1D<ArrayType>::Backward(
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
  ArrayType input_error(input.shape());
  ArrayType kernel_error(kernels.shape());

  SizeType horizontal_stride_width  = kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  ArrayType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  ArrayType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, input_channels, kernel_height);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height);

  // Reshape error_signal to error for matmul
  ArrayType error{{vertical_stride_width, horizontal_stride_height}};
  ReverseFillOutput(error, error_signal, output_channels, output_height);

  // Backwards matmul
  ArrayType error2 = fetch::math::DotTranspose(error, horizontal_stride);
  ArrayType error1 = fetch::math::TransposeDot(vertical_stride, error);

  // Reshape horizontal stride to input data error_signal - reversed im2col
  ReverseFillHorizontalStride(input_error, error1, output_height, input_channels, kernel_height);

  // Reshape vertical stride to kernel data error_signal - reversed im2col
  ReverseFillVerticalStride(kernel_error, error2, output_channels, input_channels, kernel_height);

  return {input_error, kernel_error};
}

template <class ArrayType>
std::vector<typename ArrayType::SizeType> Convolution1D<ArrayType>::ComputeOutputShape(
    std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
{
  // Return pre-computed value if exist
  if (output_shape_.size() != 0)
    return output_shape_;
  // output_shape_[0]=number of output channels
  output_shape_.emplace_back(inputs.at(1).get().shape()[0]);
  // output_shape_[1]=number of stride_size steps over input size
  output_shape_.emplace_back(
      (inputs.at(0).get().shape()[1] - inputs.at(1).get().shape()[2] + stride_size_) /
      stride_size_);
  return output_shape_;
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
 */
template <class ArrayType>
void Convolution1D<ArrayType>::FillVerticalStride(ArrayType const &input,
                                                  ArrayType &      vertical_stride,
                                                  SizeType const   output_channels,
                                                  SizeType const   input_channels,
                                                  SizeType const   kernel_height)
{
  SizeType j_s = 0;                                      // stride height iterator
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
  {

    for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
    {
      for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
      {

        vertical_stride.Set(i_oc, j_s, input.At(i_oc, i_ic, i_k));
      }
      ++j_s;
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
 */
template <class ArrayType>
void Convolution1D<ArrayType>::ReverseFillVerticalStride(ArrayType &      input,
                                                         ArrayType const &vertical_stride,
                                                         SizeType const   output_channels,
                                                         SizeType const   input_channels,
                                                         SizeType const   kernel_height)
{
  SizeType j_s = 0;                                      // stride height iterator
  assert(input.shape().size() == 3);
  assert(vertical_stride.shape().size() == 2);
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
  {

    for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
    {
      for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
      {

        input(i_oc, i_ic, i_k) = vertical_stride(i_oc, j_s);
      }
      ++j_s;
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
 * @param input_channels
 * @param kernel_height
 */
template <class ArrayType>
void Convolution1D<ArrayType>::FillHorizontalStride(ArrayType const &input,
                                                    ArrayType &      horizontal_stride,
                                                    SizeType const   output_height,
                                                    SizeType const   input_channels,
                                                    SizeType const   kernel_height)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index
  assert(horizontal_stride.shape().size() == 2);
  assert(input.shape().size() == 2);

  j_s = 0;
  for (SizeType i_o=0; i_o < output_height; ++i_o)  // Iterate over output height
  {

    i_s = 0;
    for (SizeType i_ic=0; i_ic < input_channels; ++i_ic)  // Iterate over input channels
    {

      for (SizeType i_k=0; i_k < kernel_height; i_k++)  // Iterate over kernel height
      {
        horizontal_stride(i_s, j_s) = input(i_ic, i_o * stride_size_ + i_k);
        ++i_s;
      }
    }
    ++j_s;
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes horizontal_stride tensor to kernel(input) tensor using reversed im2col
 * @tparam ArrayType
 * @param input
 * @param horizontal_stride
 * @param output_height
 * @param input_channels
 * @param kernel_height
 */
template <class ArrayType>
void Convolution1D<ArrayType>::ReverseFillHorizontalStride(ArrayType &      input,
                                                           ArrayType const &horizontal_stride,
                                                           SizeType const   output_height,
                                                           SizeType const   input_channels,
                                                           SizeType const   kernel_height)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index

  j_s = 0;
  for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
  {

    i_s = 0;
    for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
    {

      for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
      {
        input(i_ic, i_o * stride_size_ + i_k) = horizontal_stride(i_s, j_s);
        ++i_s;
      }
    }
    ++j_s;
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
 */
template <class ArrayType>
void Convolution1D<ArrayType>::FillOutput(ArrayType const &gemm_output, ArrayType &output,
                                          SizeType const output_channels,
                                          SizeType const output_height)
{
  SizeType i_tmp;
  for (SizeType i_oc=0; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    i_tmp = 0;
    for (SizeType i_o=0; i_o < output_height; ++i_o)  // Iterate over output height
    {
      output(i_oc, i_o) =  gemm_output(i_oc, i_tmp);
      ++i_tmp;
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
 */
template <class ArrayType>
void Convolution1D<ArrayType>::ReverseFillOutput(ArrayType &gemm_output, ArrayType const &output,
                                                 SizeType const output_channels,
                                                 SizeType const output_height)
{
  SizeType i_tmp;
  for (SizeType i_oc=0; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    i_tmp = 0;
    for (SizeType i_o=0; i_o < output_height; ++i_o)  // Iterate over output height
    {
      gemm_output(i_oc, i_tmp) = output(i_oc, i_o);
      ++i_tmp;
    }
  }
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
