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

#include "ml/ops/max_pool_1d.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
MaxPool1D<T>::MaxPool1D(const SPType &sp)
  : Ops<T>(sp)
{
  kernel_size_ = sp.kernel_size;
  stride_size_ = sp.stride_size;
}

template <typename T>
std::shared_ptr<OpsSaveableParams> MaxPool1D<T>::GetOpSaveableParams()
{
  auto sp         = std::make_shared<SPType>();
  sp->kernel_size = kernel_size_;
  sp->stride_size = stride_size_;

  // Add base class savable params
  auto ops_sp  = Ops<TensorType>::GetOpSaveableParams();
  auto cast_sp = std::static_pointer_cast<OpsSaveableParams>(sp);
  *cast_sp     = *(std::static_pointer_cast<OpsSaveableParams>(ops_sp));

  return sp;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MaxPool1D<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * Applies 1D max pooling of kernel_size_ for each channel described here:
 * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height]
 * @param output tensor of size [input_channels=output_channels x number_of_stride_sized_steps]
 * @return: output tensor parameter
 */
template <typename T>
void MaxPool1D<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  // Input must be a 3D tensor [C x W x N]
  assert(inputs.at(0)->shape().size() == 3);
  assert(output.shape() == ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  SizeType iter;
  DataType max;
  DataType val;
  auto     out_it = output.begin();

  for (SizeType n_i{0}; n_i < output.shape().at(2); n_i++)  // iterate over batch
  {
    // output_channels = input_channels
    for (SizeType i{0}; i < output.shape().at(1); i++)  // Iterate over kernel stride
    {
      iter = i * stride_size_;
      for (SizeType c{0}; c < output.shape().at(0); ++c)  // Iterate over output channels
      {
        max = inputs.at(0)->At(c, iter, n_i);

        // Get maximum value on kernel_size_ window
        for (SizeType j{1}; j < kernel_size_; j++)  // Iterate over kernel width
        {
          val = inputs.at(0)->At(c, iter + j, n_i);
          if (val > max)
          {
            max = val;
          }
        }

        // Set maximum value for each [kernel_size_] window to output
        *out_it = max;
        ++out_it;
      }
    }
  }
}

/**
 * Computes gradient of 1D max pooling of kernel_size_ for each channel described here:
 * http://ais.uni-bonn.de/papers/icann2010_maxpool.pdf
 * Error signal of max pool is passed only to max node
 * @param inputs vector of tensor references where at:
 * inputs[0] = input_data[input_channels x input_height]
 * @param error_signal tensor of size [output_channels=input_channels x
 * number_of_stride_sized_steps]
 * @return: output vector of tensors with back propagated error signal
 * output[0]=input_error[inputs[0].shape]
 */
template <typename TensorType>
std::vector<TensorType> MaxPool1D<TensorType>::Backward(const VecTensorType &inputs,
                                                        const TensorType &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() ==
         ComputeOutputShape(fetch::ml::utilities::TensorPtrsToSizes(inputs)));

  TensorType return_signal{inputs.at(0)->shape()};

  auto output_shape = error_signal.shape();

  SizeType iter;
  DataType max;
  DataType val;
  SizeType max_iter;
  auto     er_it = error_signal.cbegin();

  for (SizeType n_i{0}; n_i < output_shape.at(2); n_i++)  // iterate over batch
  {
    for (SizeType i{0}; i < output_shape.at(1); i++)  // Iterate over kernel stride
    {
      iter = i * stride_size_;
      for (SizeType c{0}; c < output_shape.at(0); ++c)  // Iterate over output channels
      {
        max      = inputs.at(0)->At(c, iter, n_i);
        max_iter = iter;

        // Find max node
        for (SizeType j{0}; j < kernel_size_; j++)  // Iterate over kernel width
        {
          val = inputs.at(0)->At(c, iter + j, n_i);
          if (val > max)
          {
            max      = val;
            max_iter = iter + j;
          }
        }

        // Add error to max node
        return_signal.Set(c, max_iter, n_i, return_signal.At(c, max_iter, n_i) + *er_it);
        ++er_it;
      }
    }
  }

  return {return_signal};
}

template <typename T>
std::vector<fetch::math::SizeType> MaxPool1D<T>::ComputeOutputShape(
    const std::vector<math::SizeVector> &inputs) const
{
  std::vector<SizeType> output_shape;

  // output_shape_[0]=number of output channels
  output_shape.emplace_back(inputs.at(0).at(0));
  // output_shape_[1]=number of stride_size steps over input size
  SizeType output_height = ((inputs.at(0).at(1) - kernel_size_) / stride_size_) + 1;
  if (output_height == 0 || output_height == std::numeric_limits<SizeType>::max())
  {
    throw fetch::math::exceptions::WrongShape(
        "MaxPool1D::ComputeOutputShape: output shape has 0 or -1 values!");
  }
  output_shape.emplace_back(output_height);
  // output_shape_[2]=batch dimension
  output_shape.emplace_back(inputs.at(0).at(2));
  return output_shape;
}

template <typename TensorType>
OperationsCount MaxPool1D<TensorType>::ChargeForward() const
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::MAX_PER_ELEMENT *
                         this->batch_output_shape_.at(0) * this->batch_output_shape_.at(1) *
                         this->batch_output_shape_.at(2) * this->kernel_size_;
  return cost;
}

template <typename TensorType>
std::pair<OperationsCount, math::SizeVector> MaxPool1D<TensorType>::ChargeBackward(
    std::vector<math::SizeVector> const &input_shapes)
{
  assert(!this->batch_output_shape_.empty());
  OperationsCount cost = fetch::ml::charge_estimation::ops::MAX_PER_ELEMENT *
                         this->batch_output_shape_.at(0) * this->batch_output_shape_.at(1) *
                         this->batch_output_shape_.at(2) * this->kernel_size_;
  math::SizeVector output_shape = ComputeOutputShape(input_shapes);
  return std::make_pair(cost * output_shape.back(), output_shape);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class MaxPool1D<math::Tensor<int8_t>>;
template class MaxPool1D<math::Tensor<int16_t>>;
template class MaxPool1D<math::Tensor<int32_t>>;
template class MaxPool1D<math::Tensor<int64_t>>;
template class MaxPool1D<math::Tensor<float>>;
template class MaxPool1D<math::Tensor<double>>;
template class MaxPool1D<math::Tensor<fixed_point::fp32_t>>;
template class MaxPool1D<math::Tensor<fixed_point::fp64_t>>;
template class MaxPool1D<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
