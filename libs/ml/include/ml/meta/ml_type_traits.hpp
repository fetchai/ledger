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

/////////////////////////////////////////////////////////////
///  DETERMINING OP/LAYER FROM A SAVE PARAMETER DESCRIPTOR///
/////////////////////////////////////////////////////////////

template <OpType OperationType>
static constexpr bool IsAbs = (OperationType == OpType::ABS);
template <OpType OperationType>
static constexpr bool IsAdd = (OperationType == OpType::ADD);
template <OpType OperationType>
static constexpr bool IsConcatenate = (OperationType == OpType::CONCATENATE);
template <OpType OperationType>
static constexpr bool IsConvolution1D = (OperationType == OpType::CONVOLUTION_1D);
template <OpType OperationType>
static constexpr bool IsConvolution2D = (OperationType == OpType::CONVOLUTION_2D);
template <OpType OperationType>
static constexpr bool IsCrossEntropyLoss = (OperationType == OpType::CROSS_ENTROPY_LOSS);
template <OpType OperationType>
static constexpr bool IsDivide = (OperationType == OpType::DIVIDE);
template <OpType OperationType>
static constexpr bool IsDropout = (OperationType == OpType::DROPOUT);
template <OpType OperationType>
static constexpr bool IsElu = (OperationType == OpType::ELU);
template <OpType OperationType>
static constexpr bool IsEmbeddings = (OperationType == OpType::EMBEDDINGS);
template <OpType OperationType>
static constexpr bool IsExp = (OperationType == OpType::EXP);
template <OpType OperationType>
static constexpr bool IsFlatten = (OperationType == OpType::FLATTEN);
template <OpType OperationType>
static constexpr bool IsGraphOp = (OperationType == OpType::GRAPH);
template <OpType OperationType>
static constexpr bool IsLayerConvolution = (OperationType == OpType::LAYER_CONVOLUTION);
template <OpType OperationType>
static constexpr bool IsLayerFullyConnected = (OperationType == OpType::LAYER_FULLY_CONNECTED);
template <OpType OperationType>
static constexpr bool IsLeakyRelu = (OperationType == OpType::LEAKY_RELU);
template <OpType OperationType>
static constexpr bool IsLeakyReluOp = (OperationType == OpType::LEAKY_RELU_OP);
template <OpType OperationType>
static constexpr bool IsLog = (OperationType == OpType::LOG);
template <OpType OperationType>
static constexpr bool IsLogSigmoid = (OperationType == OpType::LOGSIGMOID);
template <OpType OperationType>
static constexpr bool IsLogSoftmax = (OperationType == OpType::LOGSOFTMAX);
template <OpType OperationType>
static constexpr bool IsMatrixMultiply = (OperationType == OpType::MATRIX_MULTIPLY);
template <OpType OperationType>
static constexpr bool IsMaxPool = (OperationType == OpType::MAX_POOL);
template <OpType OperationType>
static constexpr bool IsMeanSquareErrorLoss = (OperationType == OpType::MEAN_SQUARE_ERROR_LOSS);
template <OpType OperationType>
static constexpr bool IsMaximum = (OperationType == OpType::MAXIMUM);
template <OpType OperationType>
static constexpr bool IsMultiply = (OperationType == OpType::MULTIPLY);
template <OpType OperationType>
static constexpr bool IsPlaceholder = (OperationType == OpType::PLACEHOLDER);
template <OpType OperationType>
static constexpr bool IsRandomisedRelu = (OperationType == OpType::RANDOMISED_RELU);
template <OpType OperationType>
static constexpr bool IsRelu = (OperationType == OpType::RELU);
template <OpType OperationType>
static constexpr bool IsReshape = (OperationType == OpType::RESHAPE);
template <OpType OperationType>
static constexpr bool IsSigmoid = (OperationType == OpType::SIGMOID);
template <OpType OperationType>
static constexpr bool IsSoftmax = (OperationType == OpType::SOFTMAX);
template <OpType OperationType>
static constexpr bool IsSoftmaxCrossEntropyLoss = (OperationType ==
                                                   OpType::SOFTMAX_CROSS_ENTROPY_LOSS);
template <OpType OperationType>
static constexpr bool IsSqrt = (OperationType == OpType::SQRT);
template <OpType OperationType>
static constexpr bool IsSubgraph = (OperationType == OpType::SUBGRAPH);
template <OpType OperationType>
static constexpr bool IsSubtract = (OperationType == OpType::SUBTRACT);
template <OpType OperationType>
static constexpr bool IsTanh = (OperationType == OpType::TANH);
template <OpType OperationType>
static constexpr bool IsTranspose = (OperationType == OpType::TRANSPOSE);
template <OpType OperationType>
static constexpr bool IsWeights = (OperationType == OpType::WEIGHTS);

template <OpType OperationType, typename R = void>
using IfOpIsAbs = fetch::meta::EnableIf<IsAbs<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsAdd = fetch::meta::EnableIf<IsAdd<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsConcatenate = fetch::meta::EnableIf<IsConcatenate<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsConvolution1D = fetch::meta::EnableIf<IsConvolution1D<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsConvolution2D = fetch::meta::EnableIf<IsConvolution2D<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsCrossEntropyLoss = fetch::meta::EnableIf<IsCrossEntropyLoss<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsDivide = fetch::meta::EnableIf<IsDivide<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsDropout = fetch::meta::EnableIf<IsDropout<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsElu = fetch::meta::EnableIf<IsElu<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsEmbeddings = fetch::meta::EnableIf<IsEmbeddings<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsExp = fetch::meta::EnableIf<IsExp<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsFlatten = fetch::meta::EnableIf<IsFlatten<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsGraph = fetch::meta::EnableIf<IsGraphOp<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLayerConvolution = fetch::meta::EnableIf<IsLayerConvolution<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLayerFullyConnceted = fetch::meta::EnableIf<IsLayerFullyConnected<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLeakyRelu = fetch::meta::EnableIf<IsLeakyRelu<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLeakyReluOp = fetch::meta::EnableIf<IsLeakyReluOp<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLog = fetch::meta::EnableIf<IsLog<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLogSigmoid = fetch::meta::EnableIf<IsLogSigmoid<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsLogSoftmax = fetch::meta::EnableIf<IsLogSoftmax<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsMatrixMultiply = fetch::meta::EnableIf<IsMatrixMultiply<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsMaxPool = fetch::meta::EnableIf<IsMaxPool<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsMeanSquareErrorLoss = fetch::meta::EnableIf<IsMeanSquareErrorLoss<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsMaximum = fetch::meta::EnableIf<IsMaximum<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsMultiply = fetch::meta::EnableIf<IsMultiply<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsPlaceholder = fetch::meta::EnableIf<IsPlaceholder<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsRandomisedRelu = fetch::meta::EnableIf<IsRandomisedRelu<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsRelu = fetch::meta::EnableIf<IsRelu<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsReshape = fetch::meta::EnableIf<IsReshape<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsSigmoid = fetch::meta::EnableIf<IsSigmoid<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsSoftmax = fetch::meta::EnableIf<IsSoftmax<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsSoftmaxCrossEntropyLoss =
    fetch::meta::EnableIf<IsSoftmaxCrossEntropyLoss<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsSqrt = fetch::meta::EnableIf<IsSqrt<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsSubgraph = fetch::meta::EnableIf<IsSubgraph<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsSubtract = fetch::meta::EnableIf<IsSubtract<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsTanh = fetch::meta::EnableIf<IsTanh<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsTranspose = fetch::meta::EnableIf<IsTranspose<OperationType>, R>;
template <OpType OperationType, typename R = void>
using IfOpIsWeights = fetch::meta::EnableIf<IsWeights<OperationType>, R>;

}  // namespace meta
}  // namespace ml
}  // namespace fetch
