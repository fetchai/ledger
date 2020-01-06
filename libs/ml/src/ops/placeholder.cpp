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

#include "ml/ops/dataholder.hpp"
#include "ml/ops/placeholder.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
PlaceHolder<TensorType>::PlaceHolder(SPType const &sp) : DataHolder<TensorType>(sp)
{}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> PlaceHolder<TensorType>::GetOpSaveableParams()
{
  return std::make_shared<SPType>();
}

/**
 * Placeholders should not be shared, therefore a layer sharing its elements
 * with another node should use a new (unshared) placeholder op
 * @param me
 * @return
 */
template <typename TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> PlaceHolder<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);

  if (this->data_)
  {
    copyshare->data_ = std::make_shared<TensorType>(this->data_->Copy());
  }

  return copyshare;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class PlaceHolder<math::Tensor<int8_t>>;
template class PlaceHolder<math::Tensor<int16_t>>;
template class PlaceHolder<math::Tensor<int32_t>>;
template class PlaceHolder<math::Tensor<int64_t>>;
template class PlaceHolder<math::Tensor<uint8_t>>;
template class PlaceHolder<math::Tensor<uint16_t>>;
template class PlaceHolder<math::Tensor<uint32_t>>;
template class PlaceHolder<math::Tensor<uint64_t>>;
template class PlaceHolder<math::Tensor<float>>;
template class PlaceHolder<math::Tensor<double>>;
template class PlaceHolder<math::Tensor<fixed_point::fp32_t>>;
template class PlaceHolder<math::Tensor<fixed_point::fp64_t>>;
template class PlaceHolder<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
