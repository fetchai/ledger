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
#include "ml/ops/convolution_1d.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Convolution1D<TensorType>::GetOpSaveableParams()
{
  auto sp         = std::make_shared<SPType>();
  sp->stride_size = stride_size_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Convolution1D<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * Applies 1D convolution using im2col with General Matrix Multiplication described here:
 * https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height], inputs[1] = kernel_data[kernel_channels x
 * kernel_height x batch_position]
 * @param output tensor of size [output_channels x number_of_stride_sized_steps x batch_position]
 * @return: output tensor parameter
 */
template <class TensorType>
void Convolution1D<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  // Input should be a 3D tensor [C x H x N]
  assert(inputs.at(0)->shape().size() == 3);
  // Kernels should be a 4D tensor [oC x iC x H x N]
  assert(inputs.at(1)->shape().size() == 4);
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  // input data channels = kernel input channels
  assert(inputs.at(0)->shape().at(0) == inputs.at(1)->shape().at(1));

  TensorType input   = (*inputs.at(0));
  TensorType kernels = (*inputs.at(1));

  SizeType input_channels  = input.shape().at(0);
  SizeType batch_size      = input.shape().at(2);
  SizeType output_channels = kernels.shape().at(0);
  SizeType kernel_height   = kernels.shape().at(2);
  SizeType output_height   = output.shape().at(1);

  SizeType horizontal_stride_width  = kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * batch_size;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  TensorType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  TensorType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, input_channels, kernel_height,
                       batch_size);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height);

  // Do matmul
  TensorType reshaped_output = fetch::math::Dot(vertical_stride, horizontal_stride);

  // Reshape values after matmul to output
  FillOutput(reshaped_output, output, output_channels, output_height, batch_size);
}

/**
 * Computes gradient of 1D convolution using reversed im2col and General Matrix Multiplication
 * described here: https://www.scss.tcd.ie/~andersan/static/papers/asap-2017.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height], inputs[1] = kernel_data[kernel_channels x
 * kernel_height x batch_position]
 * @param error_signal tensor of size [output_channels x number_of_stride_sized_steps x
 * batch_position]
 * @return: output vector of tensors with back propagated error signal
 * output[0]=input_error[inputs[0].shape], output[1]=kernel_error[inputs[1].shape]
 */
template <class TensorType>
std::vector<TensorType> Convolution1D<TensorType>::Backward(VecTensorType const &inputs,
                                                            TensorType const &   error_signal)
{
  assert(inputs.size() == 2);
  // Input should be a 2D tensor [C x H x N]
  assert(inputs.at(0)->shape().size() == 3);
  // Kernels should be a 3D tensor [oC x iC x H x N]
  assert(inputs.at(1)->shape().size() == 4);
  assert(error_signal.shape() ==
         ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  SizeType output_height = error_signal.shape().at(1);

  TensorType input   = (*inputs.at(0));
  TensorType kernels = (*inputs.at(1));

  SizeType   input_channels  = input.shape().at(0);
  SizeType   batch_size      = input.shape().at(2);
  SizeType   output_channels = kernels.shape().at(0);
  SizeType   kernel_height   = kernels.shape().at(2);
  TensorType input_error(input.shape());
  TensorType kernel_error(kernels.shape());

  SizeType horizontal_stride_width  = kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * batch_size;
  SizeType vertical_stride_width    = output_channels;

  // Horizontal stride contains input data
  TensorType horizontal_stride{{horizontal_stride_width, horizontal_stride_height}};
  // Vertical stride contains kernel data
  TensorType vertical_stride{{vertical_stride_width, horizontal_stride_width}};

  // Reshape input data to horizontal stride - im2col
  FillHorizontalStride(input, horizontal_stride, output_height, input_channels, kernel_height,
                       batch_size);

  // Reshape kernel data to vertical stride - im2col
  FillVerticalStride(kernels, vertical_stride, output_channels, input_channels, kernel_height);

  // Reshape error_signal to error for matmul
  TensorType error{{vertical_stride_width, horizontal_stride_height}};
  ReverseFillOutput(error, error_signal, output_channels, output_height, batch_size);

  // Backwards matmul
  TensorType error2 = fetch::math::DotTranspose(error, horizontal_stride);
  TensorType error1 = fetch::math::TransposeDot(vertical_stride, error);

  // Reshape horizontal stride to input data error_signal - reversed im2col
  ReverseFillHorizontalStride(input_error, error1, output_height, input_channels, kernel_height,
                              batch_size);

  // Reshape vertical stride to kernel data error_signal - reversed im2col
  ReverseFillVerticalStride(kernel_error, error2, output_channels, input_channels, kernel_height);

  return {input_error, kernel_error};
}

template <class TensorType>
std::vector<typename TensorType::SizeType> Convolution1D<TensorType>::ComputeOutputShape(
    std::vector<math::SizeVector> const &inputs) const
{
  std::vector<SizeType> output_shape;

  // output_shape_[0]=number of output channels
  output_shape.emplace_back(inputs.at(1).at(0));
  // output_shape_[1]=number of stride_size steps over input size
  output_shape.emplace_back(ComputeOutputHeight(inputs.at(0).at(1), inputs.at(1).at(2)));
  // output_shape_[2]=batch dimension
  output_shape.emplace_back(inputs.at(0).at(2));

  return output_shape;
}

template <class TensorType>
math::SizeType Convolution1D<TensorType>::ComputeOutputHeight(SizeType const input_height,
                                                              SizeType const kernel_height) const
{
  // output_height=number of stride_size steps over input size
  SizeType output_height = ((input_height - kernel_height) / this->stride_size_) + 1;
  if (output_height == 0 || output_height == std::numeric_limits<SizeType>::max())
  {
    throw fetch::math::exceptions::WrongShape(
        "Convolution1D::ComputeOutputHeight: output shape has 0 or -1 values!");
  }
  return output_height;
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
 */
template <class TensorType>
void Convolution1D<TensorType>::FillVerticalStride(TensorType const &input,
                                                   TensorType &      vertical_stride,
                                                   SizeType const    output_channels,
                                                   SizeType const    input_channels,
                                                   SizeType const    kernel_height)
{
  SizeType j_s = 0;                                      // stride height iterator
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
  {

    for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
    {
      for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
      {
        vertical_stride(i_oc, j_s) = input.At(i_oc, i_ic, i_k, 0);
      }
      ++j_s;
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
 */
template <class TensorType>
void Convolution1D<TensorType>::ReverseFillVerticalStride(TensorType &      input,
                                                          TensorType const &vertical_stride,
                                                          SizeType const    output_channels,
                                                          SizeType const    input_channels,
                                                          SizeType const    kernel_height)
{
  SizeType j_s = 0;  // stride height iterator
  assert(input.shape().size() == 4);
  assert(vertical_stride.shape().size() == 2);
  for (SizeType i_ic{0}; i_ic < input_channels; ++i_ic)  // Iterate over input channels
  {
    for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
    {
      for (SizeType i_oc{0}; i_oc < output_channels; ++i_oc)  // Iterate over output channels
      {
        input(i_oc, i_ic, i_k, 0) =
            static_cast<DataType>(input(i_oc, i_ic, i_k, 0) + vertical_stride(i_oc, j_s));
      }
      ++j_s;
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
 * @param input_channels
 * @param kernel_height
 */
template <class TensorType>
void Convolution1D<TensorType>::FillHorizontalStride(
    TensorType const &input, TensorType &horizontal_stride, SizeType const output_height,
    SizeType const input_channels, SizeType const kernel_height, SizeType const batch_size)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index
  assert(horizontal_stride.shape().size() == 2);
  assert(input.shape().size() == 3);

  j_s = 0;

  for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
  {
    for (SizeType i_o = 0; i_o < output_height; ++i_o)  // Iterate over output height
    {

      i_s = 0;
      for (SizeType i_ic = 0; i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {

        for (SizeType i_k = 0; i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          horizontal_stride(i_s, j_s) = input(i_ic, i_o * stride_size_ + i_k, i_b);
          ++i_s;
        }
      }

      ++j_s;
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
 * @param input_channels
 * @param kernel_height
 */
template <class TensorType>
void Convolution1D<TensorType>::ReverseFillHorizontalStride(
    TensorType &input, TensorType const &horizontal_stride, SizeType const output_height,
    SizeType const input_channels, SizeType const kernel_height, SizeType const batch_size)
{
  SizeType i_s;  // stride width index
  SizeType j_s;  // stride height index

  j_s = 0;
  for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
  {

    for (SizeType i_o{0}; i_o < output_height; ++i_o)  // Iterate over output height
    {
      i_s = 0;

      for (SizeType i_ic(0); i_ic < input_channels; ++i_ic)  // Iterate over input channels
      {
        for (SizeType i_k(0); i_k < kernel_height; i_k++)  // Iterate over kernel height
        {
          input(i_ic, i_o * stride_size_ + i_k, i_b) = horizontal_stride(i_s, j_s);
          ++i_s;
        }
      }
      ++j_s;
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
 */
template <class TensorType>
void Convolution1D<TensorType>::FillOutput(TensorType const &gemm_output, TensorType &output,
                                           SizeType const output_channels,
                                           SizeType const output_height, SizeType const batch_size)
{
  SizeType i_it;
  for (SizeType i_oc = 0; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    i_it = 0;
    for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
    {
      for (SizeType i_o = 0; i_o < output_height; ++i_o)  // Iterate over output height
      {
        output(i_oc, i_o, i_b) = gemm_output(i_oc, i_it);
        ++i_it;
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
 */
template <class TensorType>
void Convolution1D<TensorType>::ReverseFillOutput(TensorType &gemm_output, TensorType const &output,
                                                  SizeType const output_channels,
                                                  SizeType const output_height,
                                                  SizeType const batch_size)
{
  SizeType i_it;
  for (SizeType i_oc = 0; i_oc < output_channels; ++i_oc)  // Iterate over output channels
  {
    i_it = 0;
    for (SizeType i_b{0}; i_b < batch_size; ++i_b)  // Iterate over batch
    {
      for (SizeType i_o = 0; i_o < output_height; ++i_o)  // Iterate over output height
      {
        gemm_output(i_oc, i_it) = output(i_oc, i_o, i_b);
        ++i_it;
      }
    }
  }
}

template <typename TensorType>
OperationsCount Convolution1D<TensorType>::ChargeForward() const
{
  assert(!this->batch_output_shape_.empty());
  assert(this->batch_input_shapes_.size() == 2);

  SizeType input_channels  = this->batch_input_shapes_.front().at(0);
  SizeType batch_size      = this->batch_input_shapes_.front().at(2);
  SizeType output_channels = this->batch_input_shapes_.back().at(0);
  SizeType kernel_height   = this->batch_input_shapes_.back().at(2);

  SizeType output_height = ComputeOutputHeight(this->batch_input_shapes_.front().at(1),
                                               this->batch_input_shapes_.back().at(2));

  SizeType horizontal_stride_width  = kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * batch_size;
  SizeType vertical_stride_width    = output_channels;

  OperationsCount cost = horizontal_stride_width * horizontal_stride_height *
                         vertical_stride_width *
                         fetch::ml::charge_estimation::ops::MULTIPLICATION_PER_ELEMENT;

  return cost;
}

template <typename TensorType>
OperationsCount Convolution1D<TensorType>::ChargeBackward() const
{
  assert(!this->batch_output_shape_.empty());
  assert(this->batch_input_shapes_.size() == 2);

  SizeType input_channels  = this->batch_input_shapes_.front().at(0);
  SizeType batch_size      = this->batch_input_shapes_.front().at(2);
  SizeType output_channels = this->batch_input_shapes_.back().at(0);
  SizeType kernel_height   = this->batch_input_shapes_.back().at(2);

  SizeType output_height = ComputeOutputHeight(this->batch_input_shapes_.front().at(1),
                                               this->batch_input_shapes_.back().at(2));

  SizeType horizontal_stride_width  = kernel_height * input_channels;
  SizeType horizontal_stride_height = output_height * batch_size;
  SizeType vertical_stride_width    = output_channels;

  OperationsCount cost =
      2 * (horizontal_stride_width * horizontal_stride_height * vertical_stride_width *
           fetch::ml::charge_estimation::ops::MULTIPLICATION_PER_ELEMENT);

  return cost;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Convolution1D<math::Tensor<int8_t>>;
template class Convolution1D<math::Tensor<int16_t>>;
template class Convolution1D<math::Tensor<int32_t>>;
template class Convolution1D<math::Tensor<int64_t>>;
template class Convolution1D<math::Tensor<float>>;
template class Convolution1D<math::Tensor<double>>;
template class Convolution1D<math::Tensor<fixed_point::fp32_t>>;
template class Convolution1D<math::Tensor<fixed_point::fp64_t>>;
template class Convolution1D<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
