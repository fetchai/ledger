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
namespace optimisers {

static constexpr OperationsCount SGD_N_CACHES      = 1;
static constexpr OperationsCount SGD_STEP_INIT     = 8;
static constexpr OperationsCount SGD_PER_TRAINABLE = 2;
static constexpr OperationsCount SGD_SPARSE_APPLY  = 2;

static constexpr OperationsCount MOMENTUM_N_CACHES      = 2;
static constexpr OperationsCount MOMENTUM_STEP_INIT     = 8;
static constexpr OperationsCount MOMENTUM_PER_TRAINABLE = 5;

static constexpr OperationsCount ADAGRAD_N_CACHES      = 2;
static constexpr OperationsCount ADAGRAD_STEP_INIT     = 8;
static constexpr OperationsCount ADAGRAD_PER_TRAINABLE = 8;

static constexpr OperationsCount RMSPROP_N_CACHES      = 2;
static constexpr OperationsCount RMSPROP_STEP_INIT     = 8;
static constexpr OperationsCount RMSPROP_PER_TRAINABLE = 10;

static constexpr OperationsCount ADAM_N_CACHES      = 5;
static constexpr OperationsCount ADAM_STEP_INIT     = 8;
static constexpr OperationsCount ADAM_PER_TRAINABLE = 15;

}  // namespace optimisers
}  // namespace charge_estimation
}  // namespace ml
}  // namespace fetch
