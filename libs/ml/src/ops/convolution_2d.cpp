//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "ml/ops/convolution_2d.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Convolution2D<TensorType>::GetOpSaveableParams()
{
  auto sp         = std::make_shared<SPType>();
  sp->stride_size = stride_size_;
  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Convolution2D<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * Applies 2D convolution using im2col with General Matrix Multiplication described here:
 * https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height x input_width x batch_position], inputs[1] =
 * kernel_data[kernel_channels x kernel_height x kernel_width x batch_position]
 * @param output tensor of size [output_channels x number_of_stride_sized_steps_over_input_height x
 * number_of_stride_sized_steps_over_input_width x batch_position]
 * @return: output tensor parameter
 */
template <class TensorType>
void Convolution2D<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  // Input should be a 4D tensor [C x H x W x N]
  assert(inputs.at(0)->shape().size() == 4);
  // Kernels should be a 5D tensor [oC x iC x H x W x N]
  assert(inputs.at(1)->shape().size() == 5);
  assert(output.shape() == ComputeOutputShape(inputs));

  TensorType input   = (*inputs.at(0));
  TensorType kernels = (*inputs.at(1));

  SizeType input_channels  = input.shape().at(0);
  SizeType batch_size      = input.shape().at(3);
  SizeType output_channels = kernels.shape().at(0);
  SizeType kernel_height   = kernels.shape().at(2);
  SizeType kernel_width    = kernels.shape().at(3);
  SizeType output_height   = output.shape().at(1);
  SizeType output_width    = output.shape().at(2);

  SizeType horizontal_stride_width  = kernel_width * kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * output_width * batch_size;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  TensorType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  TensorType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, output_width, input_channels,
                       kernel_height, kernel_width, batch_size);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height,
                     kernel_width);

  // Do matmul
  TensorType reshaped_output = fetch::math::Dot(vertical_stride, horizontal_stride);

  // Reshape values after matmul to output
  FillOutput(reshaped_output, output, output_channels, output_height, output_width, batch_size);
}

/**
 * Computes gradient of 2D convolution using reversed im2col and General Matrix Multiplication
 * described here: https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height x input_width], inputs[1] =
 * kernel_data[kernel_channels x kernel_height x kernel_width x batch_position]
 * @param error_signal tensor of size [output_channels x
 * number_of_stride_sized_steps_over_input_height x number_of_stride_sized_steps_over_input_width x
 * batch_position]
 * @return: output vector of tensors with back propagated error signal
 * output[0]=input_error[inputs[0].shape], output[1]=kernel_error[inputs[1].shape]
 */
template <class TensorType>
std::vector<TensorType> Convolution2D<TensorType>::Backward(VecTensorType const &inputs,
                                                            TensorType const &   error_signal)
{
  assert(inputs.size() == 2);
  // Input should be a 4D tensor [C x H x W x N]
  assert(inputs.at(0)->shape().size() == 4);
  // Kernels should be a 5D tensor [oC x iC x H x W x N]
  assert(inputs.at(1)->shape().size() == 5);
  assert(error_signal.shape() == ComputeOutputShape(inputs));

  // input data channels = kernel input channels
  assert(inputs.at(0)->shape().at(0) == inputs.at(1)->shape().at(1));

  SizeType output_height = error_signal.shape().at(1);
  SizeType output_width  = error_signal.shape().at(2);

  TensorType input   = (*inputs.at(0));
  TensorType kernels = (*inputs.at(1));

  SizeType   input_channels  = input.shape().at(0);
  SizeType   batch_size      = input.shape().at(3);
  SizeType   output_channels = kernels.shape().at(0);
  SizeType   kernel_height   = kernels.shape().at(2);
  SizeType   kernel_width    = kernels.shape().at(3);
  TensorType input_error(input.shape());
  TensorType kernel_error(kernels.shape());

  SizeType horizontal_stride_width  = kernel_width * kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * output_width * batch_size;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  TensorType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  TensorType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, output_width, input_channels,
                       kernel_height, kernel_width, batch_size);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height,
                     kernel_width);

  // Reshape error_signal to error for matmul
  TensorType error{{vertical_stride_width, horizontal_stride_height}};
  ReverseFillOutput(error, error_signal, output_channels, output_height, output_width, batch_size);

  // Backwards matmul
  TensorType error2 = fetch::math::DotTranspose(error, horizontal_stride);
  TensorType error1 = fetch::math::TransposeDot(vertical_stride, error);

  // Reshape horizontal stride to input data error_signal - reversed im2col
  ReverseFillHorizontalStride(input_error, error1, output_height, output_width, input_channels,
                              kernel_height, kernel_width, batch_size);

  // Reshape vertical stride to kernel data error_signal - reversed im2col
  ReverseFillVerticalStride(kernel_error, error2, output_channels, input_channels, kernel_height,
                            kernel_width);

  return {input_error, kernel_error};
}

template <class TensorType>
std::vector<typename TensorType::SizeType> Convolution2D<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  std::vector<SizeType> output_shape;

  // output_shape_[0]=number of output channels
  output_shape.emplace_back(inputs.at(1)->shape()[0]);
  // output_shape_[1]=number of stride_size steps over input height
  output_shape.emplace_back((inputs.at(0)->shape()[1] - inputs.at(1)->shape()[2] + stride_size_) /
                            stride_size_);
  // output_shape_[2]=number of stride_size steps over input width
  output_shape.emplace_back((inputs.at(0)->shape()[2] - inputs.at(1)->shape()[3] + stride_size_) /
                            stride_size_);
  // output_shape_[3]=batch dimension
  output_shape.emplace_back(inputs.at(0)->shape().at(3));

  return output_shape;
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes input tensor to vertical_stride tensor using im2col
 * @tparam TensorType
 * @param input
 * @param vertical_stride
 * @param output_channels
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class TensorType>
void Convolution2D<TensorType>::FillVerticalStride(TensorType &input, TensorType &vertical_stride,
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
          vertical_stride(i_oc, j_s) = input.At(i_oc, i_ic, i_k, j_k, 0);
          ++j_s;
        }
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes vertical_stride tensor to input tensor using reversed im2col
 * @tparam TensorType
 * @param input
 * @param vertical_stride
 * @param output_channels
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class TensorType>
void Convolution2D<TensorType>::ReverseFillVerticalStride(
    TensorType &input, TensorType &vertical_stride, SizeType const output_channels,
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
          input(i_oc, i_ic, i_k, j_k, 0) = vertical_stride.At(i_oc, j_s);
          ++j_s;
        }
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes kernel(input) tensor to horizontal_stride tensor using im2col
 * @tparam TensorType
 * @param input
 * @param horizontal_stride
 * @param output_height
 * @param output_width
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class TensorType>
void Convolution2D<TensorType>::FillHorizontalStride(
    TensorType &input, TensorType &horizontal_stride, SizeType const output_height,
    SizeType const output_width, SizeType const input_channels, SizeType const kernel_height,
    SizeType const kernel_width, SizeType const batch_size)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index

  j_s = 0;
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
  {
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
              horizontal_stride(i_s, j_s) =
                  input.At(i_ic, i_o * stride_size_ + i_k, j_o * stride_size_ + j_k, i_b);
              ++i_s;
            }
          }
        }
        ++j_s;
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshapes horizontal_stride tensor to kernel(input) tensor using reversed im2col
 * @tparam TensorType
 * @param input
 * @param horizontal_stride
 * @param output_height
 * @param output_width
 * @param input_channels
 * @param kernel_height
 * @param kernel_width
 */
template <class TensorType>
void Convolution2D<TensorType>::ReverseFillHorizontalStride(
    TensorType &input, TensorType &horizontal_stride, SizeType const output_height,
    SizeType const output_width, SizeType const input_channels, SizeType const kernel_height,
    SizeType const kernel_width, SizeType const batch_size)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index

  j_s = 0;
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
  {
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
              input(i_ic, i_o * stride_size_ + i_k, j_o * stride_size_ + j_k, i_b) =
                  horizontal_stride.At(i_s, j_s);
              ++i_s;
            }
          }
        }
        ++j_s;
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshape gemm_output tensor (result of matmul on vertical and horizontal stride) to output tensor
 * @tparam TensorType
 * @param gemm_output
 * @param output
 * @param output_channels
 * @param output_height
 * @param output_width
 */
template <class TensorType>
void Convolution2D<TensorType>::FillOutput(TensorType const &gemm_output, TensorType &output,
                                           SizeType const output_channels,
                                           SizeType const output_height,
                                           SizeType const output_width, SizeType const batch_size)
{

  SizeType it;
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    it = 0;
    for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
    {
      for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
      {
        for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
        {
          output(i_oc, i_o, j_o, i_b) = gemm_output.At(i_oc, it);
          ++it;
        }
      }
    }
  }
}

// TODO(issue 943): Make im2col efficient using iterators
/**
 * Reshape output tensor to gemm_output tensor (result of matmul on vertical and horizontal stride)
 * @tparam TensorType
 * @param gemm_output
 * @param output
 * @param output_channels
 * @param output_height
 * @param output_width
 */
template <class TensorType>
void Convolution2D<TensorType>::ReverseFillOutput(TensorType &gemm_output, TensorType const &output,
                                                  SizeType const output_channels,
                                                  SizeType const output_height,
                                                  SizeType const output_width,
                                                  SizeType const batch_size)
{

  SizeType it;
  for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    it = 0;
    for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
    {
      for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
      {
        for (SizeType j_o{0}; j_o < output_width; ++j_o)  // Iterate over output width
        {
          gemm_output(i_oc, it) = output.At(i_oc, i_o, j_o, i_b);
          ++it;
        }
      }
    }
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Convolution2D<math::Tensor<int8_t>>;
template class Convolution2D<math::Tensor<int16_t>>;
template class Convolution2D<math::Tensor<int32_t>>;
template class Convolution2D<math::Tensor<int64_t>>;
template class Convolution2D<math::Tensor<uint8_t>>;
template class Convolution2D<math::Tensor<uint16_t>>;
template class Convolution2D<math::Tensor<uint32_t>>;
template class Convolution2D<math::Tensor<uint64_t>>;
template class Convolution2D<math::Tensor<float>>;
template class Convolution2D<math::Tensor<double>>;
template class Convolution2D<math::Tensor<fixed_point::fp32_t>>;
template class Convolution2D<math::Tensor<fixed_point::fp64_t>>;
template class Convolution2D<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
