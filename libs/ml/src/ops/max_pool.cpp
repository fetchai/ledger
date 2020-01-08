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

#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/max_pool.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {
/**
 * Update pointer to pooling op depending on input size
 * @tparam TensorType
 * @param inputs
 */
template <typename TensorType>
void MaxPool<TensorType>::UpdatePointer(VecTensorType const &inputs)
{
  switch (inputs.at(0)->shape().size())
  {
    // MaxPool1D
  case static_cast<SizeType>(3):
  {
    if (!pool_op_ptr_ || pool_2d_)
    {
      pool_op_ptr_ =
          std::make_shared<fetch::ml::ops::MaxPool1D<TensorType>>(kernel_size_, stride_size_);
      pool_2d_ = false;
    }
    break;
  }
    // MaxPool2D
  case static_cast<SizeType>(4):
  {
    if (!pool_op_ptr_ || !pool_2d_)
    {
      pool_op_ptr_ =
          std::make_shared<fetch::ml::ops::MaxPool2D<TensorType>>(kernel_size_, stride_size_);
      pool_2d_ = true;
    }
    break;
  }

  default:
  {
    throw fetch::ml::exceptions::InvalidMode("Unsupported data shape");
  }
  }
}

template <class T>
MaxPool<T>::MaxPool(const SPType &sp)
  : Ops<T>(sp)
{
  kernel_size_ = sp.kernel_size;
  stride_size_ = sp.stride_size;
}

template <class T>
std::shared_ptr<OpsSaveableParams> MaxPool<T>::GetOpSaveableParams()
{
  SPType sp{};
  sp.kernel_size = kernel_size_;
  sp.stride_size = stride_size_;
  return std::make_shared<SPType>(sp);
}

template <class TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MaxPool<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class T>
void MaxPool<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 1);

  UpdatePointer(inputs);
  pool_op_ptr_->Forward(inputs, output);
}

template <class TensorType>
std::vector<TensorType> MaxPool<TensorType>::Backward(const VecTensorType &inputs,
                                                      const TensorType &   error_signal)
{
  assert(inputs.size() == 1);

  UpdatePointer(inputs);
  return pool_op_ptr_->Backward(inputs, error_signal);
}

template <class TensorType>
std::vector<fetch::math::SizeType> MaxPool<TensorType>::ComputeOutputShape(
    const VecTensorType &inputs) const
{
  assert(inputs.size() == 1);
  assert(inputs.at(0)->shape().size() == 3 || inputs.at(0)->shape().size() == 4);

  std::vector<SizeType> output_shape;

  // output_shape_[0]=number of output channels
  output_shape.emplace_back(inputs.at(0)->shape().at(0));
  // output_shape_[1]=number of stride_size steps over input height
  output_shape.emplace_back((inputs.at(0)->shape().at(1) - (kernel_size_ - stride_size_)) /
                            stride_size_);

  // MaxPool1D
  if (inputs.at(0)->shape().size() == 3)
  {
    // output_shape_[2]=batch dimension
    output_shape.emplace_back(inputs.at(0)->shape().at(2));
  }
  // MaxPool2D
  else
  {
    // output_shape_[2]=number of stride_size steps over input width
    output_shape.emplace_back((inputs.at(0)->shape().at(2) - (kernel_size_ - stride_size_)) /
                              stride_size_);
    // output_shape_[3]=batch dimension
    output_shape.emplace_back(inputs.at(0)->shape().at(3));
  }

  return output_shape;
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
