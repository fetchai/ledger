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

#include "ml/ops/trainable.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <typename TensorType>
void Trainable<TensorType>::SetRegularisation(RegPtrType regulariser, DataType regularisation_rate)
{
  regulariser_         = regulariser;
  regularisation_rate_ = regularisation_rate;
}

/**
 * Enable or disable trainable gradient update freezing
 * @param new_frozen_state
 */
template <typename TensorType>
void Trainable<TensorType>::SetFrozenState(bool new_frozen_state)
{
  value_frozen_ = new_frozen_state;
}

template <typename TensorType>
bool Trainable<TensorType>::GetFrozenState() const
{
  return value_frozen_;
}
///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Trainable<math::Tensor<int8_t>>;
template class Trainable<math::Tensor<int16_t>>;
template class Trainable<math::Tensor<int32_t>>;
template class Trainable<math::Tensor<int64_t>>;
template class Trainable<math::Tensor<uint8_t>>;
template class Trainable<math::Tensor<uint16_t>>;
template class Trainable<math::Tensor<uint32_t>>;
template class Trainable<math::Tensor<uint64_t>>;
template class Trainable<math::Tensor<float>>;
template class Trainable<math::Tensor<double>>;
template class Trainable<math::Tensor<fixed_point::fp32_t>>;
template class Trainable<math::Tensor<fixed_point::fp64_t>>;
template class Trainable<math::Tensor<fixed_point::fp128_t>>;
//

}  // namespace ops
}  // namespace ml
}  // namespace fetch
