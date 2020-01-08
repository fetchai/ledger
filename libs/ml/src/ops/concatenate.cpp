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

#include "ml/ops/concatenate.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Concatenate<TensorType>::GetOpSaveableParams()
{
  auto sp_ptr  = std::make_shared<SPType>();
  sp_ptr->axis = axis_;
  return sp_ptr;
}

template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Concatenate<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

/**
 * concatenates multiple input tensors into one
 */
template <typename TensorType>
void Concatenate<TensorType>::Forward(VecTensorType const &inputs, TensorType &output)
{
  std::vector<TensorType> tensors;
  for (auto const &e : inputs)
  {
    tensors.emplace_back(*e);
  }

  output = TensorType::Concat(tensors, axis_);
}

/**
 * Splits up the gradients to feed back to the inputs
 * @param inputs
 * @param error_signal
 * @return
 */
template <typename TensorType>
std::vector<TensorType> Concatenate<TensorType>::Backward(VecTensorType const &inputs,
                                                          TensorType const &   error_signal)
{
  concat_points_.resize(inputs.size());
  auto c_it = concat_points_.begin();
  for (auto const &e : inputs)
  {
    *c_it = e->shape()[axis_];
    ++c_it;
  }

  return TensorType::Split(error_signal, concat_points_, axis_);
}

template <typename TensorType>
std::vector<math::SizeType> Concatenate<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  std::vector<SizeType> ret_shape{inputs.front()->shape()};
  for (std::size_t i = 1; i < inputs.size(); i++)
  {
    ret_shape[axis_] += inputs.at(i)->shape(axis_);
  }

  return ret_shape;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Concatenate<math::Tensor<int8_t>>;
template class Concatenate<math::Tensor<int16_t>>;
template class Concatenate<math::Tensor<int32_t>>;
template class Concatenate<math::Tensor<int64_t>>;
template class Concatenate<math::Tensor<uint8_t>>;
template class Concatenate<math::Tensor<uint16_t>>;
template class Concatenate<math::Tensor<uint32_t>>;
template class Concatenate<math::Tensor<uint64_t>>;
template class Concatenate<math::Tensor<float>>;
template class Concatenate<math::Tensor<double>>;
template class Concatenate<math::Tensor<fixed_point::fp32_t>>;
template class Concatenate<math::Tensor<fixed_point::fp64_t>>;
template class Concatenate<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
