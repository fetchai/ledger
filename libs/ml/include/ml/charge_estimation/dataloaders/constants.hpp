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
namespace dataloaders {

static constexpr OperationsCount TENSOR_GETNEXT_SETUP          = 600;
static constexpr OperationsCount TENSOR_GETNEXT_LABEL_COPY     = 2;
static constexpr OperationsCount TENSOR_GETNEXT_DATA_COPY      = 3;
static constexpr OperationsCount TENSOR_GETNEXT_DATA_PER_INPUT = 400;
static constexpr OperationsCount TENSOR_ADDDATA_SETUP          = 400;
static constexpr OperationsCount TENSOR_ADDDATA_LABEL_COPY     = 2;
static constexpr OperationsCount TENSOR_ADDDATA_DATA_COPY      = 2;
static constexpr OperationsCount TENSOR_ADDDATA_DATA_PER_INPUT = 300;

}  // namespace dataloaders
}  // namespace charge_estimation
}  // namespace ml
}  // namespace fetch
