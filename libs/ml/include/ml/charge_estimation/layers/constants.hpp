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

#include "ml/charge_estimation/types.hpp"

namespace fetch {
namespace ml {
namespace charge_estimation {
namespace layers {

static constexpr OperationsCount FULLY_CONNECTED_SHAPE_DEDUCTION                      = 5;
static constexpr OperationsCount FULLY_CONNECTED_SHAPE_DEDUCTION_TIME_DISTRIBUTED     = 1;
static constexpr OperationsCount FULLY_CONNECTED_SHAPE_DEDUCTION_NON_TIME_DISTRIBUTED = 5;

static constexpr OperationsCount FULLY_CONNECTED_CHARGE_CONSTRUCT                = 4;
static constexpr OperationsCount FULLY_CONNECTED_CHARGE_CONSTRUCT_NOT_AUTODETECT = 5;

static constexpr OperationsCount FULLY_CONNECTED_CHARGE_COMPILE_PER_NODE = 2;

}  // namespace layers
}  // namespace charge_estimation
}  // namespace ml
}  // namespace fetch
