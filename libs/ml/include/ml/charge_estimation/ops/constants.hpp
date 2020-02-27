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
namespace ops {

// ML OP Charge estimations allow for none piecewise discontinuity to match benchmarked performance
// better An upper cap is also implemented to prevent especially expensive operations
static constexpr OperationsCount PIECEWISE_LOWER_THRESHOLD = 524288;    // 2^14 * 32 (padded size)
static constexpr OperationsCount PIECEWISE_HARD_CAP        = 33554432;  // 2^20 * 32 (padded size)

static constexpr OperationsCount OP_OVERHEAD = 60;

static constexpr OperationsCount FIBONNACI_GENERATOR_PER_ELEMENT = 1;
static constexpr OperationsCount TANH_PER_ELEMENT                = 1;
static constexpr OperationsCount COSH_PER_ELEMENT                = 1;
static constexpr OperationsCount MAX_PER_ELEMENT                 = 1;
static constexpr OperationsCount LOW_ADDITION_PER_ELEMENT        = 5;
static constexpr OperationsCount HIGH_ADDITION_PER_ELEMENT       = 20;

static constexpr OperationsCount SUBTRACTION_PER_ELEMENT         = LOW_ADDITION_PER_ELEMENT;
static constexpr OperationsCount ASSIGN_PER_ELEMENT              = 5;
static constexpr OperationsCount LOW_FLATTEN_PER_ELEMENT         = ASSIGN_PER_ELEMENT;
static constexpr OperationsCount HIGH_FLATTEN_PER_ELEMENT        = 25;
static constexpr OperationsCount EXP_PER_ELEMENT                 = 1;
static constexpr OperationsCount LOG_PER_ELEMENT                 = 1;
static constexpr OperationsCount LOW_MULTIPLICATION_PER_ELEMENT  = 7;
static constexpr OperationsCount HIGH_MULTIPLICATION_PER_ELEMENT = 24;
static constexpr OperationsCount DIVISION_PER_ELEMENT            = 16;
static constexpr OperationsCount ABS_PER_ELEMENT                 = 1;
static constexpr OperationsCount CONCAT_PER_ELEMENT              = 1;
static constexpr OperationsCount SPLIT_PER_ELEMENT               = 1;
static constexpr OperationsCount VIEW_PER_ELEMENT                = 1;
static constexpr OperationsCount PLACEHOLDER_READING_PER_ELEMENT = 0;
static constexpr OperationsCount WEIGHTS_READING_PER_ELEMENT     = 0;
static constexpr OperationsCount MEAN_PER_ELEMENT                = 1;
static constexpr OperationsCount SQRT_PER_ELEMENT                = 1;
static constexpr OperationsCount ONE_HOT_PER_ELEMENT             = 1;
static constexpr OperationsCount RESHAPE_PER_ELEMENT             = 1;
static constexpr OperationsCount SLICE_PER_ELEMENT               = 1;
static constexpr OperationsCount COPY_PER_ELEMENT                = 1;
static constexpr OperationsCount TOPK_PER_ELEMENT                = 1;
static constexpr OperationsCount TRANSPOSE_PER_ELEMENT           = 1;
static constexpr OperationsCount BROADCAST_PER_ELEMENT           = 1;
static constexpr OperationsCount SQUEEZE_PER_ELEMENT   = RESHAPE_PER_ELEMENT + COPY_PER_ELEMENT;
static constexpr OperationsCount EMBEDDING_PER_ELEMENT = 2 * VIEW_PER_ELEMENT + ASSIGN_PER_ELEMENT;

static constexpr OperationsCount LOW_CROSS_ENTROPY_PER_ELEMENT  = 15;
static constexpr OperationsCount HIGH_CROSS_ENTROPY_PER_ELEMENT = 75;
static constexpr OperationsCount CEL_PIECEWISE_LOWER_THRESHOLD  = 16384;  // 2^14 * 32 (padded size)

static constexpr OperationsCount CROSS_ENTROPY_BACKWARD_PER_ELEMENT =
    SUBTRACTION_PER_ELEMENT + DIVISION_PER_ELEMENT;

static constexpr OperationsCount LOW_MEAN_SQ_ERROR_PER_ELEMENT  = 15;
static constexpr OperationsCount HIGH_MEAN_SQ_ERROR_PER_ELEMENT = 75;
static constexpr OperationsCount MSQE_PIECEWISE_LOWER_THRESHOLD = 16384;  // 2^14 * 32 (padded size)

static constexpr OperationsCount MEAN_SQ_ERROR_BACKWARD_PER_ELEMENT =
    2 * LOW_MULTIPLICATION_PER_ELEMENT + SUBTRACTION_PER_ELEMENT + DIVISION_PER_ELEMENT;
static constexpr OperationsCount SOFTMAX_PER_ELEMENT = MAX_PER_ELEMENT + SUBTRACTION_PER_ELEMENT +
                                                       EXP_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT +
                                                       DIVISION_PER_ELEMENT;
static constexpr OperationsCount SOFTMAX_BACKWARD_PER_ELEMENT =
    SOFTMAX_PER_ELEMENT + SUBTRACTION_PER_ELEMENT + 2 * LOW_MULTIPLICATION_PER_ELEMENT +
    LOW_ADDITION_PER_ELEMENT;
static constexpr OperationsCount SOFTMAX_CROSS_ENTROPY_PER_ELEMENT =
    CROSS_ENTROPY_PER_ELEMENT + SOFTMAX_PER_ELEMENT;
static constexpr OperationsCount SOFTMAX_CROSS_ENTROPY_BACKWARD_PER_ELEMENT =
    SOFTMAX_PER_ELEMENT + SUBTRACTION_PER_ELEMENT;
static constexpr OperationsCount POW_PER_ELEMENT =
    EXP_PER_ELEMENT + LOG_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount SIGMOID_PER_ELEMENT =
    EXP_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT + DIVISION_PER_ELEMENT;
static constexpr OperationsCount SIGMOID_BACKWARD_PER_ELEMENT =
    SIGMOID_PER_ELEMENT + 2 * LOW_MULTIPLICATION_PER_ELEMENT + SUBTRACTION_PER_ELEMENT;
static constexpr OperationsCount LOG_SIGMOID_PER_ELEMENT = SIGMOID_PER_ELEMENT + LOG_PER_ELEMENT;
static constexpr OperationsCount LOG_SIGMOID_BACKWARD_PER_ELEMENT =
    LOW_ADDITION_PER_ELEMENT + DIVISION_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount RELU_PER_ELEMENT          = MAX_PER_ELEMENT;
static constexpr OperationsCount RELU_BACKWARD_PER_ELEMENT = ASSIGN_PER_ELEMENT;
static constexpr OperationsCount DROPOUT_PER_ELEMENT =
    FIBONNACI_GENERATOR_PER_ELEMENT + SUBTRACTION_PER_ELEMENT + DIVISION_PER_ELEMENT +
    LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount ELU_PER_ELEMENT =
    EXP_PER_ELEMENT + SUBTRACTION_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount ELU_BACKWARD_PER_ELEMENT =
    EXP_PER_ELEMENT + 2 * LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount GELU_PER_ELEMENT =
    LOW_MULTIPLICATION_PER_ELEMENT + POW_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT +
    +LOW_ADDITION_PER_ELEMENT + TANH_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT +
    LOW_MULTIPLICATION_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount GELU_BACKWARD_PER_ELEMENT =
    +3 * POW_PER_ELEMENT + 8 * LOW_MULTIPLICATION_PER_ELEMENT + 4 * LOW_ADDITION_PER_ELEMENT +
    TANH_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT + COSH_PER_ELEMENT + DIVISION_PER_ELEMENT;
static constexpr OperationsCount LEAKY_RELU_PER_ELEMENT = MAX_PER_ELEMENT + ASSIGN_PER_ELEMENT;
static constexpr OperationsCount LEAKY_RELU_BACKWARD_PER_ELEMENT =
    LEAKY_RELU_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount RANDOMISED_RELU_PER_ELEMENT =
    FIBONNACI_GENERATOR_PER_ELEMENT + ASSIGN_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount RANDOMISED_RELU_BACKWARD_PER_ELEMENT =
    RANDOMISED_RELU_PER_ELEMENT + MAX_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount LOG_SOFTMAX_PER_ELEMENT = LOG_PER_ELEMENT + SOFTMAX_PER_ELEMENT;
static constexpr OperationsCount LOG_SOFTMAX_BACKWARD_PER_ELEMENT =
    SOFTMAX_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT +
    SUBTRACTION_PER_ELEMENT;
static constexpr OperationsCount LAYER_NORM_PER_ELEMENT =
    2 * MEAN_PER_ELEMENT + SUBTRACTION_PER_ELEMENT + POW_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT +
    SQRT_PER_ELEMENT + 2 * DIVISION_PER_ELEMENT;
static constexpr OperationsCount LAYER_NORM_BACKWARD_PER_ELEMENT =
    2 * LOW_MULTIPLICATION_PER_ELEMENT + 2 * LOW_ADDITION_PER_ELEMENT + SUBTRACTION_PER_ELEMENT +
    DIVISION_PER_ELEMENT;
static constexpr OperationsCount MASK_FILL_PER_ELEMENT =
    2 * LOW_MULTIPLICATION_PER_ELEMENT + SUBTRACTION_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT;
static constexpr OperationsCount DIVISION_BACKWARD_PER_ELEMENT =
    2 * DIVISION_PER_ELEMENT + 2 * LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount MULTIPLICATION_BACKWARD_PER_ELEMENT =
    2 * LOW_MULTIPLICATION_PER_ELEMENT + LOW_ADDITION_PER_ELEMENT + RESHAPE_PER_ELEMENT;
static constexpr OperationsCount SQRT_BACKWARD_PER_ELEMENT =
    SQRT_PER_ELEMENT + DIVISION_PER_ELEMENT + LOW_MULTIPLICATION_PER_ELEMENT;
static constexpr OperationsCount TANH_BACKWARD_PER_ELEMENT =
    TANH_PER_ELEMENT + 2 * LOW_MULTIPLICATION_PER_ELEMENT + SUBTRACTION_PER_ELEMENT;

static constexpr OperationsCount OP_DEFAULT_CONSTRUCTION_COST         = 1;
static constexpr OperationsCount OP_MATRIX_MULTIPLY_CONSTRUCTION_COST = 3;

}  // namespace ops
}  // namespace charge_estimation
}  // namespace ml
}  // namespace fetch
