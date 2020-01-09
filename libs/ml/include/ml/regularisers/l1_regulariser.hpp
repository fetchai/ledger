#pragma once
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

#include "core/assert.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/regularisers/reg_types.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace regularisers {

/*
 * L1 regularisation
 */
template <class T>
class L1Regulariser : public Regulariser<T>
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;

  L1Regulariser()
    : Regulariser<T>(RegularisationType::L1)
  {}
  ~L1Regulariser() override = default;

  /**
   * Applies regularisation gradient on specified tensor
   * L1 regularisation gradient, where w=weight, a=regularisation_rate
   * f'(w)=a*(w/|w|)
   * @param weight tensor reference
   * @param regularisation_rate
   */
  void ApplyRegularisation(TensorType &weight, DataType regularisation_rate) override
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
};

}  // namespace regularisers
}  // namespace ml
}  // namespace fetch
