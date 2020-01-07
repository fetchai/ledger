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

#include "meta/type_traits.hpp"
#include <type_traits>

namespace fetch {
namespace ml {

enum class OptimiserType : uint8_t
{
  ADAGRAD,
  ADAM,
  MOMENTUM,
  RMSPROP,
  SGD,
  LAZY_ADAM
};

enum class LoaderType : uint8_t
{
  TENSOR,
  SGNS,
  W2V,
  COMMODITY,
  C2V
};

enum class SliceType : uint8_t
{
  SINGLE_AXIS,
  MULTI_AXIS,
  RANGED
};

enum class OpKind : uint8_t
{
  INVALID,
  OP,
  LOSS,
  LAYER
};

enum class OpType : uint16_t
{
  GRAPH,

  // OpKind - INVALID
  NONE,

  // OpKind - Op
  OP_ABS,
  OP_ADD,
  OP_AVG_POOL_1D,
  OP_AVG_POOL_2D,
  OP_CONCATENATE,
  OP_CONSTANT,
  OP_CONVOLUTION_1D,
  OP_CONVOLUTION_2D,
  OP_DATAHOLDER,
  OP_DIVIDE,
  OP_DROPOUT,
  OP_ELU,
  OP_EMBEDDINGS,
  OP_EXP,
  OP_FLATTEN,
  OP_GELU,
  OP_LAYER_NORM,
  OP_LEAKY_RELU,
  OP_LOG,
  OP_LOGSIGMOID,
  OP_LOGSOFTMAX,
  OP_MASK_FILL,
  OP_MATRIX_MULTIPLY,
  OP_MAX_POOL,
  OP_MAX_POOL_1D,
  OP_MAX_POOL_2D,
  OP_MAXIMUM,
  OP_MULTIPLY,
  OP_ONE_HOT,
  OP_PLACEHOLDER,
  OP_PRELU_OP,
  OP_RANDOMISED_RELU,
  OP_REDUCE_MEAN,
  OP_RELU,
  OP_RESHAPE,
  OP_SIGMOID,
  OP_SLICE,
  OP_SOFTMAX,
  OP_SQUEEZE,
  OP_SQRT,
  OP_SUBTRACT,
  OP_STRIDED_SLICE,
  OP_SWITCH,
  OP_TANH,
  OP_TRANSPOSE,
  OP_TOP_K,
  OP_VARIABLE,
  OP_WEIGHTS,

  // OpKind - LOSS
  LOSS_CROSS_ENTROPY,
  LOSS_SOFTMAX_CROSS_ENTROPY,
  LOSS_MEAN_SQUARE_ERROR,

  // Metrics
  METRIC_CATEGORICAL_ACCURACY,

  // OpKind - LAYER
  SUBGRAPH,
  LAYER_CONVOLUTION_1D,
  LAYER_CONVOLUTION_2D,
  LAYER_FULLY_CONNECTED,
  LAYER_LAYER_NORM,
  LAYER_MULTI_HEAD_ATTENTION,
  LAYER_PRELU,
  LAYER_SCALED_DOT_PRODUCT_ATTENTION,
  LAYER_SELF_ATTENTION_ENCODER,
  LAYER_SKIP_GRAM
};

/////////////////////////////
///  FORWARD DECLARATIONS ///
/////////////////////////////

template <typename T>
class Graph;

template <typename T>
class Node;

namespace layers {
template <typename T>
class FullyConnected;
}

namespace ops {
template <typename T>
class Trainable;

template <typename T>
class Constant;
}  // namespace ops

namespace meta {

//////////////////////////////////////////////////////////
///  GRAPH & TRAINABLE / NOT-TRAINABLE NOT TYPE SFINAE ///
//////////////////////////////////////////////////////////

template <typename T, typename OperationType>
constexpr bool IsFullyConneted =
    std::is_base_of<fetch::ml::layers::FullyConnected<T>, OperationType>::value;

template <typename T, typename OperationType>
constexpr bool IsConstant = std::is_base_of<fetch::ml::ops::Constant<T>, OperationType>::value;

template <typename T, typename OperationType>
constexpr bool IsTrainable = std::is_base_of<fetch::ml::ops::Trainable<T>, OperationType>::value;

template <typename T, typename OperationType>
constexpr bool IsNotTrainable = !IsTrainable<T, OperationType>;

template <typename T, typename OperationType>
constexpr bool IsGraph = std::is_base_of<fetch::ml::Graph<T>, OperationType>::value;

template <typename T, typename OperationType>
constexpr bool IsShareable = IsFullyConneted<T, OperationType> || IsConstant<T, OperationType>;

template <typename T, typename OperationType>
constexpr bool IsNotShareable = !IsShareable<T, OperationType>;

template <typename T, typename OperationType>
constexpr bool IsNotGraph = !IsGraph<T, OperationType>;

template <typename T, typename OperationType>
constexpr bool                     IsNotGraphOrTrainable =
    IsNotGraph<T, OperationType> &&IsNotTrainable<T, OperationType>;

template <typename T, typename OperationType, typename R = void>
using IfIsTrainable = fetch::meta::EnableIf<IsTrainable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsGraph = fetch::meta::EnableIf<IsGraph<T, OperationType>, R>;

// TODO (#1397) Need to implement shared weight for every shareable weight layers and ops
template <typename T, typename OperationType, typename R = void>
using IfIsShareable = fetch::meta::EnableIf<IsShareable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotShareable = fetch::meta::EnableIf<IsNotShareable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotTrainable = fetch::meta::EnableIf<IsNotTrainable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotGraph = fetch::meta::EnableIf<IsNotGraph<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotGraphOrTrainable = fetch::meta::EnableIf<IsNotGraphOrTrainable<T, OperationType>, R>;

}  // namespace meta
}  // namespace ml
}  // namespace fetch
