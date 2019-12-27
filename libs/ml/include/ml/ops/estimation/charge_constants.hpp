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

#include "meta/vm_types.hpp"

namespace fetch {
namespace ml {
namespace ops {
namespace charge_cost {

using fetch::vm::ChargeAmount;

static constexpr ChargeAmount DROPOUT_PER_ELEMENT             = 3;
static constexpr ChargeAmount RELU_PER_ELEMENT                = 1;
static constexpr ChargeAmount SIGMOID_PER_ELEMENT             = 2;
static constexpr ChargeAmount ADDITION_PER_ELEMENT            = 1;
static constexpr ChargeAmount FLATTEN_PER_ELEMENT             = 1;
static constexpr ChargeAmount MULTIPLICATION_PER_ELEMENT      = 3;
static constexpr ChargeAmount PLACEHOLDER_READING_PER_ELEMENT = 0;
static constexpr ChargeAmount WEIGHTS_READING_PER_ELEMENT     = 0;

}  // namespace charge_cost
}  // namespace ops
}  // namespace ml
}  // namespace fetch
