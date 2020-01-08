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

template <class T>
MaxPool1D<T>::MaxPool1D(const SPType &sp)
  : Ops<T>(sp)
{
  kernel_size_ = sp.kernel_size;
  stride_size_ = sp.stride_size;
}

template <class T>
std::shared_ptr<OpsSaveableParams> MaxPool1D<T>::GetOpSaveableParams()
{
  SPType sp{};
  sp.kernel_size = kernel_size_;
  sp.stride_size = stride_size_;
  return std::make_shared<SPType>(sp);
}

template <class TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MaxPool1D<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class T>
void MaxPool1D<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  // Input must be a 3D tensor [C x W x N]
  assert(inputs.at(0)->shape().size() == 3);
  assert(output.shape() == ComputeOutputShape(inputs));

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

template <class TensorType>
std::vector<TensorType> MaxPool1D<TensorType>::Backward(const VecTensorType &inputs,
                                                        const TensorType &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() == ComputeOutputShape(inputs));

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

template <class T>
std::vector<fetch::math::SizeType> MaxPool1D<T>::ComputeOutputShape(
    const VecTensorType &inputs) const
{
  std::vector<SizeType> output_shape;

  // output_shape_[0]=number of output channels
  output_shape.emplace_back(inputs.at(0)->shape().at(0));
  // output_shape_[1]=number of stride_size steps over input size
  output_shape.emplace_back((inputs.at(0)->shape().at(1) - (kernel_size_ - stride_size_)) /
                            stride_size_);
  // output_shape_[2]=batch dimension
  output_shape.emplace_back(inputs.at(0)->shape().at(2));
  return output_shape;
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
