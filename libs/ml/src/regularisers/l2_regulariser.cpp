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

#include "math/tensor/tensor.hpp"
#include "ml/regularisers/l2_regulariser.hpp"

namespace fetch {
namespace ml {
namespace regularisers {

template <typename TensorType>
L2Regulariser<TensorType>::L2Regulariser()
  : Regulariser<TensorType>(RegularisationType::L2)
{}

/**
 * Applies regularisation gradient on specified tensor
 * L2 regularisation gradient, where w=weight, a=regularisation_rate
 * f'(w)=a*(2*w)
 * @param weight tensor reference
 * @param regularisation_rate
 */
template <typename TensorType>
void L2Regulariser<TensorType>::ApplyRegularisation(TensorType &weight,
                                                    DataType    regularisation_rate)
{
  auto coef = static_cast<DataType>(2 * regularisation_rate);

  auto it = weight.begin();
  while (it.is_valid())
  {
    *it = static_cast<DataType>(*it - ((*it) * coef));
    ++it;
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class L2Regulariser<math::Tensor<int8_t>>;
template class L2Regulariser<math::Tensor<int16_t>>;
template class L2Regulariser<math::Tensor<int32_t>>;
template class L2Regulariser<math::Tensor<int64_t>>;
template class L2Regulariser<math::Tensor<uint8_t>>;
template class L2Regulariser<math::Tensor<uint16_t>>;
template class L2Regulariser<math::Tensor<uint32_t>>;
template class L2Regulariser<math::Tensor<uint64_t>>;
template class L2Regulariser<math::Tensor<float>>;
template class L2Regulariser<math::Tensor<double>>;
template class L2Regulariser<math::Tensor<fixed_point::fp32_t>>;
template class L2Regulariser<math::Tensor<fixed_point::fp64_t>>;
template class L2Regulariser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace regularisers
}  // namespace ml
}  // namespace fetch
