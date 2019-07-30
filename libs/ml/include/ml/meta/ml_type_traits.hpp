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

#include "meta/type_traits.hpp"
#include <type_traits>

namespace fetch {
namespace ml {

enum class OpType : uint16_t
{
  ABS,
  ADD,
  CONCATENATE,
  CONVOLUTION_1D,
  CONVOLUTION_2D,
  CROSS_ENTROPY_LOSS,
  DIVIDE,
  DROPOUT,
  ELU,
  EMBEDDINGS,
  EXP,
  FLATTEN,
  GRAPH,
  LAYER_CONVOLUTION,
  LAYER_FULLY_CONNECTED,
  LEAKY_RELU,
  LEAKY_RELU_OP,
  LOG,
  LOGSIGMOID,
  LOGSOFTMAX,
  MATRIX_MULTIPLY,
  MAX_POOL,
  MEAN_SQUARE_ERROR_LOSS,
  MAXIMUM,
  MULTIPLY,
  NONE,
  PLACEHOLDER,
  RANDOMISED_RELU,
  RELU,
  RESHAPE,
  SIGMOID,
  SOFTMAX,
  SOFTMAX_CROSS_ENTROPY_LOSS,
  SQRT,
  SUBGRAPH,
  SUBTRACT,
  TANH,
  TRANSPOSE,
  WEIGHTS
};

/////////////////////////////
///  FORWARD DECLARATIONS ///
/////////////////////////////

template <typename T>
class Graph;

namespace ops {
template <typename T>
class Trainable;

template <typename T>
class Abs;

}  // namespace ops

namespace meta {

//////////////////////////////////////////////////////////
///  GRAPH & TRAINABLE / NOT-TRAINABLE NOT TYPE SFINAE ///
//////////////////////////////////////////////////////////

template <typename T, typename OperationType>
constexpr bool IsTrainable = std::is_base_of<fetch::ml::ops::Trainable<T>, OperationType>::value;

template <typename T, typename OperationType>
constexpr bool IsNotTrainable = !IsTrainable<T, OperationType>;

template <typename T, typename OperationType>
constexpr bool IsGraph = std::is_base_of<fetch::ml::Graph<T>, OperationType>::value;

template <typename T, typename OperationType>
constexpr bool IsNotGraph = !IsGraph<T, OperationType>;

template <typename T, typename OperationType>
constexpr bool                     IsNotGraphOrTrainable =
    IsNotGraph<T, OperationType> &&IsNotTrainable<T, OperationType>;

template <typename T, typename OperationType, typename R = void>
using IfIsTrainable = fetch::meta::EnableIf<IsTrainable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsGraph = fetch::meta::EnableIf<IsGraph<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotTrainable = fetch::meta::EnableIf<IsNotTrainable<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotGraph = fetch::meta::EnableIf<IsNotGraph<T, OperationType>, R>;

template <typename T, typename OperationType, typename R = void>
using IfIsNotGraphOrTrainable = fetch::meta::EnableIf<IsNotGraphOrTrainable<T, OperationType>, R>;

}  // namespace meta
}  // namespace ml
}  // namespace fetch
