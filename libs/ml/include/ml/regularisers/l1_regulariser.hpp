#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "math/tensor.hpp"
#include "ml/regularisers/regulariser.hpp"
#include <memory>
#include <vector>

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
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;

  ~L1Regulariser() = default;
  void ApplyRegularisation(ArrayType &weight, DataType regularisation_rate) override
  {
    DataType coef = static_cast<DataType>(-2) * regularisation_rate;

    auto it = weight.begin();
    while (it.is_valid())
    {
      *it *= coef;
      ++it;
    }
  }
};

}  // namespace regularisers
}  // namespace ml
}  // namespace fetch
