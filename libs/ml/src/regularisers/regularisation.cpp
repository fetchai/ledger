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
#include "ml/regularisers/l1_regulariser.hpp"
#include "ml/regularisers/l2_regulariser.hpp"
#include "ml/regularisers/reg_types.hpp"
#include "ml/regularisers/regularisation.hpp"
#include <memory>

namespace fetch {
namespace ml {
namespace Regularisations {

template <class T>
std::shared_ptr<fetch::ml::regularisers::Regulariser<T>> CreateRegulariser(RegularisationType type)
{
  using RegType    = fetch::ml::regularisers::Regulariser<T>;
  using RegPtrType = std::shared_ptr<RegType>;

  RegPtrType ret;
  switch (type)
  {
  case RegularisationType::NONE:
    // No regulariser
    break;

  case RegularisationType::L1:
    ret.reset(new fetch::ml::regularisers::L1Regulariser<T>());
    break;

  case RegularisationType::L2:
    ret.reset(new fetch::ml::regularisers::L2Regulariser<T>());
    break;

  default:
    throw exceptions::InvalidMode("Unknown regulariser type");
  }
  return ret;
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<int8_t>>>
    CreateRegulariser<math::Tensor<int8_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<int16_t>>>
    CreateRegulariser<math::Tensor<int16_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<int32_t>>>
    CreateRegulariser<math::Tensor<int32_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<int64_t>>>
    CreateRegulariser<math::Tensor<int64_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<uint8_t>>>
    CreateRegulariser<math::Tensor<uint8_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<uint16_t>>>
    CreateRegulariser<math::Tensor<uint16_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<uint32_t>>>
    CreateRegulariser<math::Tensor<uint32_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<uint64_t>>>
    CreateRegulariser<math::Tensor<uint64_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<float>>>
    CreateRegulariser<math::Tensor<float>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<double>>>
    CreateRegulariser<math::Tensor<double>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<fixed_point::fp32_t>>>
    CreateRegulariser<math::Tensor<fixed_point::fp32_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<fixed_point::fp64_t>>>
    CreateRegulariser<math::Tensor<fixed_point::fp64_t>>(RegularisationType);
template std::shared_ptr<fetch::ml::regularisers::Regulariser<math::Tensor<fixed_point::fp128_t>>>
    CreateRegulariser<math::Tensor<fixed_point::fp128_t>>(RegularisationType);

}  // namespace Regularisations
}  // namespace ml
}  // namespace fetch
