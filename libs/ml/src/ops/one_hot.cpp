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

#include "math/one_hot.hpp"
#include "ml/ops/one_hot.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
OneHot<T>::OneHot(const OneHot::SPType &sp)
  : Ops<T>(sp)
{
  depth_     = sp.depth;
  axis_      = sp.axis;
  on_value_  = sp.on_value;
  off_value_ = sp.off_value;
}

template <typename T>
std::shared_ptr<OpsSaveableParams> OneHot<T>::GetOpSaveableParams()
{
  SPType sp{};
  sp.depth     = depth_;
  sp.axis      = axis_;
  sp.on_value  = on_value_;
  sp.off_value = off_value_;

  return std::make_shared<SPType>(sp);
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> OneHot<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<OneHot::TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <typename T>
void OneHot<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  fetch::math::OneHot(output, *(inputs.at(0)), depth_, axis_, on_value_, off_value_);
}

template <typename TensorType>
std::vector<TensorType> OneHot<TensorType>::Backward(const VecTensorType &inputs,
                                                     const TensorType &   error_signal)
{
  FETCH_UNUSED(error_signal);
  assert(inputs.size() == 1);
  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  // No derivative defined for OneHotOp
  return {TensorType{inputs.at(0)->shape()}};
}

template <typename T>
std::vector<fetch::math::SizeType> OneHot<T>::ComputeOutputShape(
    const OneHot::VecTensorType &inputs) const
{
  assert(inputs.size() == 1);

  std::vector<SizeType> shape = inputs.at(0)->shape();

  if (axis_ == shape.size())
  {
    shape.emplace_back(depth_);
  }
  else
  {
    shape.insert(shape.begin() + static_cast<math::PtrDiffType>(axis_), depth_);
  }

  return shape;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class OneHot<math::Tensor<int8_t>>;
template class OneHot<math::Tensor<int16_t>>;
template class OneHot<math::Tensor<int32_t>>;
template class OneHot<math::Tensor<int64_t>>;
template class OneHot<math::Tensor<uint8_t>>;
template class OneHot<math::Tensor<uint16_t>>;
template class OneHot<math::Tensor<uint32_t>>;
template class OneHot<math::Tensor<uint64_t>>;
template class OneHot<math::Tensor<float>>;
template class OneHot<math::Tensor<double>>;
template class OneHot<math::Tensor<fixed_point::fp32_t>>;
template class OneHot<math::Tensor<fixed_point::fp64_t>>;
template class OneHot<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
