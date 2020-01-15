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
#include "ml/regularisers/l1_regulariser.hpp"
#include "ml/regularisers/reg_types.hpp"

namespace fetch {
namespace ml {
namespace regularisers {

template <typename TensorType>
L1Regulariser<TensorType>::L1Regulariser()
  : Regulariser<TensorType>(RegularisationType::L1)
{}

/**
 * Applies regularisation gradient on specified tensor
 * L1 regularisation gradient, where w=weight, a=regularisation_rate
 * f'(w)=a*(w/|w|)
 * @param weight tensor reference
 * @param regularisation_rate
 */
template <typename TensorType>
void L1Regulariser<TensorType>::ApplyRegularisation(TensorType &weight,
                                                    DataType    regularisation_rate)
{
  auto it = weight.begin();
  while (it.is_valid())
  {
    if (*it > DataType{0})
    {
      *it = static_cast<DataType>(*it - regularisation_rate);
    }
    else
    {
      *it = static_cast<DataType>(*it + regularisation_rate);
    }
    ++it;
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class L1Regulariser<math::Tensor<int8_t>>;
template class L1Regulariser<math::Tensor<int16_t>>;
template class L1Regulariser<math::Tensor<int32_t>>;
template class L1Regulariser<math::Tensor<int64_t>>;
template class L1Regulariser<math::Tensor<uint8_t>>;
template class L1Regulariser<math::Tensor<uint16_t>>;
template class L1Regulariser<math::Tensor<uint32_t>>;
template class L1Regulariser<math::Tensor<uint64_t>>;
template class L1Regulariser<math::Tensor<float>>;
template class L1Regulariser<math::Tensor<double>>;
template class L1Regulariser<math::Tensor<fixed_point::fp32_t>>;
template class L1Regulariser<math::Tensor<fixed_point::fp64_t>>;
template class L1Regulariser<math::Tensor<fixed_point::fp128_t>>;

}  // namespace regularisers
}  // namespace ml
}  // namespace fetch
