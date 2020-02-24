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

#include "math/charge_estimation/types.hpp"

namespace fetch {
namespace math {
namespace charge_estimation {

static constexpr OperationsCount TENSOR_ITERATION_DEFAULT    = 3;
static constexpr OperationsCount TENSOR_ITERATION_END_OF_ROW = 6;

}  // namespace charge_estimation
}  // namespace math
}  // namespace fetch
