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

namespace fetch {
namespace ml {
namespace regularisers {

/*
 * Abstract Regulariser interface
 */
template <class T>
class Regulariser
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;

  Regulariser() = default;

  explicit Regulariser(RegularisationType reg_type)
    : reg_type(reg_type)
  {}

  virtual ~Regulariser() = default;

  virtual void ApplyRegularisation(TensorType &weight, DataType regularisation_rate) = 0;

  RegularisationType reg_type{RegularisationType::NONE};
};

}  // namespace regularisers
}  // namespace ml
}  // namespace fetch
