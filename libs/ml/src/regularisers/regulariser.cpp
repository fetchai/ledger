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

#include "ml/regularisers/regulariser.hpp"

namespace fetch {
namespace ml {
namespace regularisers {

template <typename TensorType>
Regulariser<TensorType>::Regulariser(RegularisationType reg_type)
  : reg_type(reg_type)
{}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Regulariser<math::Tensor<int8_t>>;
template class Regulariser<math::Tensor<int16_t>>;
template class Regulariser<math::Tensor<int32_t>>;
template class Regulariser<math::Tensor<int64_t>>;
template class Regulariser<math::Tensor<uint8_t>>;
template class Regulariser<math::Tensor<uint16_t>>;
template class Regulariser<math::Tensor<uint32_t>>;
template class Regulariser<math::Tensor<uint64_t>>;
template class Regulariser<math::Tensor<float>>;
template class Regulariser<math::Tensor<double>>;
template class Regulariser<math::Tensor<fixed_point::fp32_t>>;
template class Regulariser<math::Tensor<fixed_point::fp64_t>>;
template class Regulariser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace regularisers
}  // namespace ml
}  // namespace fetch
