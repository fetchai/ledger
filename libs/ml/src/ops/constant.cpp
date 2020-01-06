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
#include "ml/ops/constant.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> Constant<TensorType>::GetOpSaveableParams()
{
  auto sp = std::make_shared<SPType>();
  if (this->data_)
  {
    sp->data = std::make_shared<TensorType>(this->data_->Copy());
  }
  return sp;
}

/**
 * shares the constant
 * @param me
 * @return
 */
template <typename TensorType>
std::shared_ptr<Ops<TensorType>> Constant<TensorType>::MakeSharedCopy(
    std::shared_ptr<Ops<TensorType>> me)
{
  assert(me.get() == this);
  return me;
}

/**
 * sets the internally stored data
 * @param data
 * @return
 */
template <typename TensorType>
bool Constant<TensorType>::SetData(TensorType const &data)
{
  if (!data_set_once_)
  {
    data_set_once_ = true;
    return DataHolder<TensorType>::SetData(data);
  }

  throw ml::exceptions::InvalidMode("cannot set data in constant more than once");
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Constant<math::Tensor<int8_t>>;
template class Constant<math::Tensor<int16_t>>;
template class Constant<math::Tensor<int32_t>>;
template class Constant<math::Tensor<int64_t>>;
template class Constant<math::Tensor<uint8_t>>;
template class Constant<math::Tensor<uint16_t>>;
template class Constant<math::Tensor<uint32_t>>;
template class Constant<math::Tensor<uint64_t>>;
template class Constant<math::Tensor<float>>;
template class Constant<math::Tensor<double>>;
template class Constant<math::Tensor<fixed_point::fp32_t>>;
template class Constant<math::Tensor<fixed_point::fp64_t>>;
template class Constant<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
