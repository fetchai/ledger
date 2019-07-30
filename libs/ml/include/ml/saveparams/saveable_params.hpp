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

#include "core/serializers/byte_array_buffer.hpp"
//#include "math/base_types.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/regularisers/regularisation.hpp"

//#include <typeinfo>

namespace fetch {
namespace ml {

///////////////////////////////////////////
/// DEFINE SERIALIZERS FOR OPTYPE ENUMS ///
///////////////////////////////////////////

template <typename S>
void Serialize(S &serializer, OpType const &operation_type)
{
  serializer << static_cast<uint16_t>(operation_type);
}

template <typename S>
void Deserialize(S &serializer, OpType &operation_type)
{
  serializer >> static_cast<uint16_t>(operation_type);
}

////////////////////////////
/// FORWARD DECLARATIONS ///
////////////////////////////

/**
 * Generic container for all the saveable params of an op.
 */
struct SaveableParamsInterface
{
  explicit SaveableParamsInterface(OpType op_type)
    : OP_DESCRIPTOR(op_type)
  {}
  virtual ~SaveableParamsInterface() = default;

  fetch::ml::OpType OP_DESCRIPTOR;
};

template <class TensorType>
struct AbsSaveableParams;

template <class TensorType>
struct AddSaveableParams;

template <class TensorType>
struct ConcatenateSaveableParams;

template <class TensorType>
struct Convolution1DSaveableParams;

template <class TensorType>
struct Convolution2DSaveableParams;

template <class TensorType>
struct CrossEntropyLossSaveableParams;

template <class TensorType>
struct DivideSaveableParams;

template <class TensorType>
struct DropoutSaveableParams;

template <class TensorType>
struct EluSaveableParams;

template <class TensorType>
struct EmbeddingsSaveableParams;

template <class TensorType>
struct ExpSaveableParams;

template <class TensorType>
struct FlattenSaveableParams;

template <class TensorType>
struct ConvolutionLayerSaveableParams;

template <class TensorType>
struct FullyConnectedSaveableParams;

template <class TensorType>
struct LeakyReluSaveableParams;

template <class TensorType>
struct LeakyReluOpSaveableParams;

template <class TensorType>
struct LogSaveableParams;

template <class TensorType>
struct LogSigmoidSaveableParams;

template <class TensorType>
struct LogSoftmaxSaveableParams;

template <class TensorType>
struct MatrixMultiplySaveableParams;

template <class TensorType>
struct MaxPoolSaveableParams;

template <class TensorType>
struct MeanSquareErrorSaveableParams;

template <class TensorType>
struct MaxPoolSaveableParams;

template <class TensorType>
struct MaximumSaveableParams;

template <class TensorType>
struct MultiplySaveableParams;

template <class TensorType>
struct PlaceholderSaveableParams;

template <class TensorType>
struct RandomisedReluSaveableParams;

template <class TensorType>
struct ReluSaveableParams;

template <class TensorType>
struct ReshapeSaveableParams;

template <class TensorType>
struct SigmoidSaveableParams;

template <class TensorType>
struct SoftmaxSaveableParams;

template <class TensorType>
struct SoftmaxCrossEntropySaveableParams;

template <class TensorType>
struct SQRTSaveableParams;

template <class TensorType>
struct SubtractSaveableParams;

template <class TensorType>
struct TanhSaveableParams;

template <class TensorType>
struct TransposeSaveableParams;

template <class TensorType>
struct WeightsSaveableParams;

///////////////////////////////////////////
/// OP SPECIFIC SERIALISATION FUNCTIONS ///
///////////////////////////////////////////

template <class S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsAbs<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<AbsSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsAbs<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<AbsSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsAdd<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<AddSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsAdd<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<AddSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsConcatenate<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<ConcatenateSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsConcatenate<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<ConcatenateSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsConvolution1D<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<Convolution1DSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsConvolution1D<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<Convolution1DSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsConvolution2D<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<Convolution2DSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsConvolution2D<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<Convolution2DSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsCrossEntropyLoss<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<CrossEntropyLossSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsCrossEntropyLoss<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<CrossEntropyLossSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsDivide<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<DivideSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsDivide<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<DivideSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsDropout<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<DropoutSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsDropout<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<DropoutSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsElu<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<EluSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsElu<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<EluSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsEmbeddings<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<EmbeddingsSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsEmbeddings<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<EmbeddingsSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsExp<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<ExpSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsExp<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<ExpSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsFlatten<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<FlattenSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsFlatten<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<FlattenSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLayerConvolution<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<ConvolutionLayerSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLayerConvolution<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<ConvolutionLayerSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLayerFullyConnceted<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<FullyConnectedSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLayerFullyConnceted<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<FullyConnectedSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLeakyRelu<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<LeakyReluSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLeakyRelu<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<LeakyReluSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLeakyReluOp<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<LeakyReluOpSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLeakyReluOp<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<LeakyReluOpSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLog<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<LogSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLog<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<LogSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLogSigmoid<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<LogSigmoidSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLogSigmoid<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<LogSigmoidSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLogSoftmax<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<LogSoftmaxSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsLogSoftmax<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<LogSoftmaxSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMatrixMultiply<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<MatrixMultiplySaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMatrixMultiply<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<MatrixMultiplySaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMaxPool<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<MaxPoolSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMaxPool<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<MaxPoolSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMeanSquareErrorLoss<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<MeanSquareErrorSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMeanSquareErrorLoss<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<MeanSquareErrorSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMaximum<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<MaximumSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMaximum<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<MaximumSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMultiply<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<MultiplySaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsMultiply<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<MultiplySaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsPlaceholder<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<PlaceholderSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsPlaceholder<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<PlaceholderSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsRandomisedRelu<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<RandomisedReluSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsRandomisedRelu<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<RandomisedReluSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsRelu<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<ReluSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsRelu<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<ReluSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsReshape<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<ReshapeSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsReshape<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<ReshapeSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSigmoid<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<SigmoidSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSigmoid<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<SigmoidSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSoftmax<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<SoftmaxSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSoftmax<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<SoftmaxSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSoftmaxCrossEntropyLoss<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(
      std::dynamic_pointer_cast<SoftmaxCrossEntropySaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSoftmaxCrossEntropyLoss<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<SoftmaxCrossEntropySaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSqrt<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<SQRTSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSqrt<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<SQRTSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSubtract<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<SubtractSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsSubtract<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<SubtractSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsTanh<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<TanhSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsTanh<OperationType, void> Deserialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> &node_pointer)
{
  auto nsp_ptr = *(std::dynamic_pointer_cast<TanhSaveableParams<TensorType>>(node_pointer));
  serializer >> nsp_ptr;
  node_pointer = (std::dynamic_pointer_cast<SaveableParamsInterface>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsTranspose<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<TransposeSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsTranspose<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<TransposeSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsWeights<OperationType, void> Serialize(
    S &serializer, std::shared_ptr<SaveableParamsInterface> const &node_pointer)
{
  serializer << *(std::dynamic_pointer_cast<WeightsSaveableParams<TensorType>>(node_pointer));
}

template <typename S, OpType OperationType, typename TensorType>
ml::meta::IfOpIsWeights<OperationType, void> Deserialize(S &serializer)
{
  auto nsp_ptr = std::make_shared<WeightsSaveableParams<TensorType>>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

///////////////////////////////
/// ALL OPS SAVEABLE PARAMS ///
///////////////////////////////

/**
 * Saveable parameters for Abs op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct AbsSaveableParams : public SaveableParamsInterface
{
  AbsSaveableParams()
    : SaveableParamsInterface(OpType::ABS)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::ABS;

  template <class S>
  friend void Serialize(S &serializer, AbsSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, AbsSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct GraphSaveableParams : public SaveableParamsInterface
{
  using DataType                        = typename TensorType::Type;
  using SizeType                        = typename TensorType::SizeType;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::GRAPH;

  //  static constexpr char const *sp_descriptor = "GraphSaveableParams";
  std::vector<std::pair<std::string, std::vector<std::string>>>   connections;
  std::map<std::string, std::shared_ptr<SaveableParamsInterface>> nodes;

  GraphSaveableParams()
    : SaveableParamsInterface(OpType::GRAPH)
  {}

  explicit GraphSaveableParams(OpType operation_type)
    : SaveableParamsInterface(operation_type)
  {}

  GraphSaveableParams &operator=(GraphSaveableParams const &gsp)
  {
    connections = gsp.connections;
    nodes       = gsp.nodes;
    return *this;
  }

  template <typename S>
  friend void Serialize(S &serializer, GraphSaveableParams const &gsp)
  {
    Serialize(serializer, gsp.OP_DESCRIPTOR);
    serializer << gsp.connections;

    for (auto const &node : gsp.nodes)
    {
      serializer << node.first;
      Serialize<S, node.second->OP_DESCRIPTOR, TensorType>(serializer, node.second);
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, GraphSaveableParams &gsp)
  {
    Deserialize(serializer, gsp.OP_DESCRIPTOR);
    serializer >> gsp.connections;
    auto num_nodes = gsp.connections.size();
    for (SizeType i = 0; i < num_nodes; i++)
    {
      std::string node_name;
      serializer >> node_name;

      std::shared_ptr<SaveableParamsInterface> nsp_ptr =
          Deserialize<S, gsp.nodes[node_name]->OP_DESCRIPTOR, TensorType>(serializer);

      gsp.nodes.insert(std::make_pair(node_name, nsp_ptr));
    }
  }
};

template <class TensorType>
struct SubGraphSaveableParams : GraphSaveableParams<TensorType>
{
  using SizeType                        = typename TensorType::SizeType;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::SUBGRAPH;

  std::vector<std::string> input_node_names;
  std::string              output_node_name;

  SubGraphSaveableParams() = default;

  explicit SubGraphSaveableParams(OpType operation_type)
    : GraphSaveableParams<TensorType>(operation_type)
  {}

  SubGraphSaveableParams &operator=(SubGraphSaveableParams const &sgsp)
  {
    input_node_names = sgsp.input_node_names;
    output_node_name = sgsp.output_node_name;
    return *this;
  }

  template <typename S>
  friend void Serialize(S &serializer, SubGraphSaveableParams const &gsp)
  {
    Serialize(serializer, gsp.OP_DESCRIPTOR);

    // serialize parent class first
    auto base_pointer = static_cast<GraphSaveableParams<TensorType> const *>(&gsp);
    Serialize(serializer, *base_pointer);

    serializer << gsp.input_node_names;
    serializer << gsp.output_node_name;
  }

  template <typename S>
  friend void Deserialize(S &serializer, SubGraphSaveableParams &gsp)
  {
    Deserialize(serializer, gsp.OP_DESCRIPTOR);

    // deserialize parent class first
    auto base_pointer = static_cast<GraphSaveableParams<TensorType> *>(&gsp);
    Deserialize(serializer, *base_pointer);

    serializer >> gsp.input_node_names;
    serializer >> gsp.output_node_name;
  }
};

/**
 * Saveable parameters for Add op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct AddSaveableParams : public SaveableParamsInterface
{
  AddSaveableParams()
    : SaveableParamsInterface(OpType::ADD)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::ADD;

  template <class S>
  friend void Serialize(S &serializer, AddSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, AddSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Concatenate op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct ConcatenateSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType   axis          = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType OP_DESCRIPTOR = OpType::CONCATENATE;

  ConcatenateSaveableParams()
    : SaveableParamsInterface(OpType::CONCATENATE)
  {}

  template <class S>
  friend void Serialize(S &serializer, ConcatenateSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.axis;
  }

  template <class S>
  friend void Deserialize(S &serializer, ConcatenateSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.axis;
  }
};

/**
 * Saveable parameters for Conv1D op
 * @tparam TensorType
 */
template <class TensorType>
struct Convolution1DSaveableParams : public SaveableParamsInterface
{
  Convolution1DSaveableParams()
    : SaveableParamsInterface(OpType::CONVOLUTION_1D)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::CONVOLUTION_1D;
  fetch::math::SizeType   stride_size   = fetch::math::numeric_max<fetch::math::SizeType>();

  template <class S>
  friend void Serialize(S &serializer, Convolution1DSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.stride_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, Convolution1DSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.stride_size;
  }
};

/**
 * Saveable parameters for Conv2D op
 * @tparam TensorType
 */
template <class TensorType>
struct Convolution2DSaveableParams : public SaveableParamsInterface
{
  Convolution2DSaveableParams()
    : SaveableParamsInterface(OpType::CONVOLUTION_2D)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::CONVOLUTION_2D;
  fetch::math::SizeType   stride_size   = fetch::math::numeric_max<fetch::math::SizeType>();

  template <class S>
  friend void Serialize(S &serializer, Convolution2DSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.stride_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, Convolution2DSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.stride_size;
  }
};

/**
 * Saveable parameters for Cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct CrossEntropyLossSaveableParams : public SaveableParamsInterface
{
  CrossEntropyLossSaveableParams()
    : SaveableParamsInterface(OpType::CROSS_ENTROPY_LOSS)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::CROSS_ENTROPY_LOSS;
  fetch::math::SizeType   stride_size   = fetch::math::numeric_max<fetch::math::SizeType>();

  template <class S>
  friend void Serialize(S &serializer, CrossEntropyLossSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.stride_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, CrossEntropyLossSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.stride_size;
  }
};

/**
 * Saveable parameters for Divide op
 * @tparam TensorType
 */
template <class TensorType>
struct DivideSaveableParams : public SaveableParamsInterface
{
  DivideSaveableParams()
    : SaveableParamsInterface(OpType::DIVIDE)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::DIVIDE;

  template <class S>
  friend void Serialize(S &serializer, DivideSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, DivideSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Dropout op
 * @tparam TensorType
 */
template <class TensorType>
struct DropoutSaveableParams : public SaveableParamsInterface
{
  DropoutSaveableParams()
    : SaveableParamsInterface(OpType::DROPOUT)
  {}

  using DataType                        = typename TensorType::Type;
  using SizeType                        = typename TensorType::SizeType;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::DROPOUT;
  SizeType                random_seed{};
  DataType                probability{};
  std::vector<uint64_t>   buffer{};
  uint64_t                index = fetch::math::numeric_max<uint64_t>();

  template <class S>
  friend void Serialize(S &serializer, DropoutSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.random_seed;
    serializer << sp.probability;
    serializer << sp.buffer;
    serializer << sp.index;
  }

  template <class S>
  friend void Deserialize(S &serializer, DropoutSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.random_seed;
    serializer >> sp.probability;
    serializer >> sp.buffer;
    serializer >> sp.index;
  }
};

/**
 * Saveable parameters for Elu op
 * @tparam TensorType
 */
template <class TensorType>
struct EluSaveableParams : public SaveableParamsInterface
{
  EluSaveableParams()
    : SaveableParamsInterface(OpType::ELU)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::ELU;

  template <class S>
  friend void Serialize(S &serializer, EluSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, EluSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Embeddings op
 * @tparam TensorType
 */
template <class TensorType>
struct EmbeddingsSaveableParams : public WeightsSaveableParams<TensorType>
{
  EmbeddingsSaveableParams()
    : WeightsSaveableParams<TensorType>(OpType::EMBEDDINGS)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::EMBEDDINGS;

  template <class S>
  friend void Serialize(S &serializer, EmbeddingsSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);

    // serialize parent class first
    auto base_pointer = static_cast<WeightsSaveableParams<TensorType> const *>(&sp);
    Serialize(serializer, *base_pointer);
  }

  template <class S>
  friend void Deserialize(S &serializer, EmbeddingsSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);

    // deserialize parent class first
    auto base_pointer = static_cast<WeightsSaveableParams<TensorType> *>(&sp);
    Deserialize(serializer, *base_pointer);
  }
};

/**
 * Saveable parameters for Exp op
 * @tparam TensorType
 */
template <class TensorType>
struct ExpSaveableParams : public SaveableParamsInterface
{
  ExpSaveableParams()
    : SaveableParamsInterface(OpType::EXP)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::EXP;

  template <class S>
  friend void Serialize(S &serializer, ExpSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, ExpSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Flatten op
 * @tparam TensorType
 */
template <class TensorType>
struct FlattenSaveableParams : public SaveableParamsInterface
{
  FlattenSaveableParams()
    : SaveableParamsInterface(OpType::FLATTEN)
  {}
  fetch::ml::OpType OP_DESCRIPTOR = OpType::FLATTEN;

  template <class S>
  friend void Serialize(S &serializer, FlattenSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, FlattenSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct ConvolutionLayerSaveableParams : SubGraphSaveableParams<TensorType>
{
  ConvolutionLayerSaveableParams() = default;

  fetch::ml::OpType OP_DESCRIPTOR = OpType::LAYER_CONVOLUTION;

  using SizeType = typename TensorType::SizeType;

  SizeType kernel_size;
  SizeType input_channels;
  SizeType output_channels;
  SizeType stride_size;

  template <typename S>
  friend void Serialize(S &serializer, ConvolutionLayerSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);

    // serialize parent class first
    auto base_pointer = static_cast<SubGraphSaveableParams<TensorType> const *>(&sp);
    Serialize(serializer, *base_pointer);

    serializer << sp.kernel_size;
    serializer << sp.input_channels;
    serializer << sp.output_channels;
    serializer << sp.stride_size;
  }

  template <typename S>
  friend void Deserialize(S &serializer, ConvolutionLayerSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);

    // deserialize parent class first
    auto base_pointer = static_cast<SubGraphSaveableParams<TensorType> *>(&sp);
    Deserialize(serializer, *base_pointer);

    serializer >> sp.kernel_size;
    serializer >> sp.input_channels;
    serializer >> sp.output_channels;
    serializer >> sp.stride_size;
  }
};

/**
 * Saveable parameters for Flatten op
 * @tparam TensorType
 */
template <class TensorType>
struct FullyConnectedSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType                        = typename TensorType::SizeType;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::LAYER_FULLY_CONNECTED;
  SizeType                in_size       = fetch::math::numeric_max<SizeType>();
  SizeType                out_size      = fetch::math::numeric_max<SizeType>();

  FullyConnectedSaveableParams()
    : SubGraphSaveableParams<TensorType>(OpType::LAYER_FULLY_CONNECTED)
  {}

  template <class S>
  friend void Serialize(S &serializer, FullyConnectedSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);

    // serialize parent class first
    auto base_pointer = static_cast<SubGraphSaveableParams<TensorType> const *>(&sp);
    Serialize(serializer, *base_pointer);

    serializer << sp.in_size;
    serializer << sp.out_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, FullyConnectedSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);

    // deserialize parent class first
    auto base_pointer = static_cast<SubGraphSaveableParams<TensorType> *>(&sp);
    Deserialize(serializer, *base_pointer);

    serializer >> sp.in_size;
    serializer >> sp.out_size;
  }
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct LeakyReluSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  DataType                a;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::LEAKY_RELU;

  LeakyReluSaveableParams()
    : SaveableParamsInterface(OpType::LEAKY_RELU)
  {}

  template <class S>
  friend void Serialize(S &serializer, LeakyReluSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.a;
  }

  template <class S>
  friend void Deserialize(S &serializer, LeakyReluSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.a;
  }
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct LeakyReluOpSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  DataType                a;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::LEAKY_RELU_OP;

  LeakyReluOpSaveableParams()
    : SaveableParamsInterface(OpType::LEAKY_RELU_OP)
  {}

  template <class S>
  friend void Serialize(S &serializer, LeakyReluOpSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.a;
  }

  template <class S>
  friend void Deserialize(S &serializer, LeakyReluOpSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.a;
  }
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct LogSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::LOG;

  LogSaveableParams()
    : SaveableParamsInterface(OpType::LOG)
  {}

  template <class S>
  friend void Serialize(S &serializer, LogSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, LogSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct LogSigmoidSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::LOGSIGMOID;

  LogSigmoidSaveableParams()
    : SaveableParamsInterface(OpType::LOGSIGMOID)
  {}

  template <class S>
  friend void Serialize(S &serializer, LogSigmoidSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.a;
  }

  template <class S>
  friend void Deserialize(S &serializer, LogSigmoidSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.a;
  }
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct LogSoftmaxSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis          = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     OP_DESCRIPTOR = OpType::LOGSOFTMAX;

  LogSoftmaxSaveableParams()
    : SaveableParamsInterface(OpType::LOGSOFTMAX)
  {}

  template <class S>
  friend void Serialize(S &serializer, LogSoftmaxSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.axis;
  }

  template <class S>
  friend void Deserialize(S &serializer, LogSoftmaxSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.axis;
  }
};

template <class TensorType>
struct MatrixMultiplySaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType OP_DESCRIPTOR = OpType::MATRIX_MULTIPLY;

  MatrixMultiplySaveableParams()
    : SaveableParamsInterface(OpType::MATRIX_MULTIPLY)
  {}

  template <class S>
  friend void Serialize(S &serializer, MatrixMultiplySaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, MatrixMultiplySaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct MaxPoolSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType kernel_size   = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size   = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     OP_DESCRIPTOR = OpType::MAX_POOL;

  MaxPoolSaveableParams()
    : SaveableParamsInterface(OpType::MAX_POOL)
  {}

  template <class S>
  friend void Serialize(S &serializer, MaxPoolSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.kernel_size;
    serializer << sp.stride_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, MaxPoolSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.kernel_size;
    serializer >> sp.stride_size;
  }
};

/**
 * Saveable parameters for MSE op
 * @tparam TensorType
 */
template <class TensorType>
struct MeanSquareErrorSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::MEAN_SQUARE_ERROR_LOSS;

  MeanSquareErrorSaveableParams()
    : SaveableParamsInterface(OpType::MEAN_SQUARE_ERROR_LOSS)
  {}

  template <class S>
  friend void Serialize(S &serializer, MeanSquareErrorSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, MeanSquareErrorSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Maximum op
 * @tparam TensorType
 */
template <class TensorType>
struct MaximumSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::MAXIMUM;

  MaximumSaveableParams()
    : SaveableParamsInterface(OpType::MAXIMUM)
  {}

  template <class S>
  friend void Serialize(S &serializer, MaximumSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, MaximumSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Multiply op
 * @tparam TensorType
 */
template <class TensorType>
struct MultiplySaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::MULTIPLY;

  MultiplySaveableParams()
    : SaveableParamsInterface(OpType::MULTIPLY)
  {}

  template <class S>
  friend void Serialize(S &serializer, MultiplySaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, MultiplySaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct PlaceholderSaveableParams : public SaveableParamsInterface
{
  std::shared_ptr<TensorType> output;
  fetch::ml::OpType           OP_DESCRIPTOR = OpType::PLACEHOLDER;

  PlaceholderSaveableParams()
    : SaveableParamsInterface(OpType::LEAKY_RELU)
  {}

  template <class S>
  friend void Serialize(S &serializer, PlaceholderSaveableParams<TensorType> const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    if (sp.output)
    {
      serializer << bool{true};
      serializer << *(sp.output);
    }
    else
    {
      serializer << bool{false};
    }
  }

  template <class S>
  friend void Deserialize(S &serializer, PlaceholderSaveableParams<TensorType> &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    bool has_weights{};
    serializer >> has_weights;
    if (has_weights)
    {
      TensorType output_temp;
      serializer >> output_temp;
      sp.output = std::make_shared<TensorType>(output_temp);
    }
  }
};

template <class TensorType>
struct RandomisedReluSaveableParams : public SaveableParamsInterface
{
  using DataType                      = typename TensorType::Type;
  using SizeType                      = typename TensorType::SizeType;
  DataType              lower_bound   = fetch::math::numeric_max<DataType>();
  DataType              upper_bound   = fetch::math::numeric_max<DataType>();
  SizeType              random_seed   = fetch::math::numeric_max<SizeType>();
  std::vector<uint64_t> buffer        = {};
  uint64_t              index         = fetch::math::numeric_max<uint64_t>();
  DataType              random_value  = fetch::math::numeric_max<DataType>();
  fetch::ml::OpType     OP_DESCRIPTOR = OpType::RANDOMISED_RELU;

  RandomisedReluSaveableParams()
    : SaveableParamsInterface(OpType::RANDOMISED_RELU)
  {}

  template <class S>
  friend void Serialize(S &serializer, RandomisedReluSaveableParams<TensorType> const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.lower_bound;
    serializer << sp.upper_bound;
    serializer << sp.random_seed;
    serializer << sp.buffer;
    serializer << sp.index;
    serializer << sp.random_value;
  }

  template <class S>
  friend void Deserialize(S &serializer, RandomisedReluSaveableParams<TensorType> &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.lower_bound;
    serializer >> sp.upper_bound;
    serializer >> sp.random_seed;
    serializer >> sp.buffer;
    serializer >> sp.index;
    serializer >> sp.random_value;
  }
};

/**
 * Saveable parameters for Relu op
 * @tparam TensorType
 */
template <class TensorType>
struct ReluSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::RELU;

  ReluSaveableParams()
    : SaveableParamsInterface(OpType::RELU)
  {}

  template <class S>
  friend void Serialize(S &serializer, ReluSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, ReluSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct ReshapeSaveableParams : public SaveableParamsInterface
{
  std::vector<fetch::math::SizeType> new_shape;
  fetch::ml::OpType                  OP_DESCRIPTOR = OpType::RESHAPE;

  ReshapeSaveableParams()
    : SaveableParamsInterface(OpType::RESHAPE)
  {}

  template <class S>
  friend void Serialize(S &serializer, ReshapeSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.new_shape;
  }

  template <class S>
  friend void Deserialize(S &serializer, ReshapeSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.new_shape;
  }
};

/**
 * Saveable parameters for Relu op
 * @tparam TensorType
 */
template <class TensorType>
struct SigmoidSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::SIGMOID;

  SigmoidSaveableParams()
    : SaveableParamsInterface(OpType::SIGMOID)
  {}

  template <class S>
  friend void Serialize(S &serializer, SigmoidSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, SigmoidSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct SoftmaxSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis          = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     OP_DESCRIPTOR = OpType::SOFTMAX;

  SoftmaxSaveableParams()
    : SaveableParamsInterface(OpType::SOFTMAX)
  {}

  template <class S>
  friend void Serialize(S &serializer, SoftmaxSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.axis;
  }

  template <class S>
  friend void Deserialize(S &serializer, SoftmaxSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.axis;
  }
};

/**
 * Saveable parameters for Softmax cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct SoftmaxCrossEntropySaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::SOFTMAX_CROSS_ENTROPY_LOSS;

  SoftmaxCrossEntropySaveableParams()
    : SaveableParamsInterface(OpType::SOFTMAX_CROSS_ENTROPY_LOSS)
  {}

  template <class S>
  friend void Serialize(S &serializer, SoftmaxCrossEntropySaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, SoftmaxCrossEntropySaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for Softmax cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct SQRTSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::SQRT;

  SQRTSaveableParams()
    : SaveableParamsInterface(OpType::SQRT)
  {}

  template <class S>
  friend void Serialize(S &serializer, SQRTSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, SQRTSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for subtract op
 * @tparam TensorType
 */
template <class TensorType>
struct SubtractSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::SUBTRACT;

  SubtractSaveableParams()
    : SaveableParamsInterface(OpType::SUBTRACT)
  {}

  template <class S>
  friend void Serialize(S &serializer, SubtractSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, SubtractSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

/**
 * Saveable parameters for subtract op
 * @tparam TensorType
 */
template <class TensorType>
struct TanhSaveableParams : public SaveableParamsInterface
{
  using DataType                  = typename TensorType::Type;
  fetch::ml::OpType OP_DESCRIPTOR = OpType::TANH;

  TanhSaveableParams()
    : SaveableParamsInterface(OpType::TANH)
  {}

  template <class S>
  friend void Serialize(S &serializer, TanhSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
  }

  template <class S>
  friend void Deserialize(S &serializer, TanhSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
  }
};

template <class TensorType>
struct TransposeSaveableParams : public SaveableParamsInterface
{
  std::vector<fetch::math::SizeType> transpose_vector;
  fetch::ml::OpType                  OP_DESCRIPTOR = OpType::TRANSPOSE;

  TransposeSaveableParams()
    : SaveableParamsInterface(OpType::TRANSPOSE)
  {}

  template <class S>
  friend void Serialize(S &serializer, TransposeSaveableParams const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    serializer << sp.transpose_vector;
  }

  template <class S>
  friend void Deserialize(S &serializer, TransposeSaveableParams &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    serializer >> sp.transpose_vector;
  }
};

template <class TensorType>
struct WeightsSaveableParams : public SaveableParamsInterface
{
  std::shared_ptr<TensorType>            output;
  fetch::ml::details::RegularisationType regularisation_type{};
  typename TensorType::Type              regularisation_rate;
  fetch::ml::OpType                      OP_DESCRIPTOR = OpType::WEIGHTS;

  WeightsSaveableParams()
    : SaveableParamsInterface(OpType::WEIGHTS)
  {}

  explicit WeightsSaveableParams(OpType operation_type)
    : SaveableParamsInterface(operation_type)
  {}

  template <class S>
  friend void Serialize(S &serializer, WeightsSaveableParams<TensorType> const &sp)
  {
    Serialize(serializer, sp.OP_DESCRIPTOR);
    if (sp.output)
    {
      serializer << bool{true};
      serializer << *(sp.output);
    }
    else
    {
      serializer << bool{false};
    }
    serializer << static_cast<int>(sp.regularisation_type);
    serializer << sp.regularisation_rate;
  }

  template <class S>
  friend void Deserialize(S &serializer, WeightsSaveableParams<TensorType> &sp)
  {
    Deserialize(serializer, sp.OP_DESCRIPTOR);
    bool has_weights{};
    serializer >> has_weights;
    if (has_weights)
    {
      TensorType output_temp;
      serializer >> output_temp;
      sp.output = std::make_shared<TensorType>(output_temp);
    }
    int reg_type_int{};
    serializer >> reg_type_int;
    sp.regularisation_type = static_cast<fetch::ml::details::RegularisationType>(reg_type_int);
    serializer >> sp.regularisation_rate;
  }
};

// template <typename S, class SP>
// void SerializeHelper(S &serializer, std::shared_ptr<SaveableParamsInterface> const &nsp)
//{
//  auto castnode = std::dynamic_pointer_cast<SP>(nsp);
//  serializer << *castnode;
//}
//
// template <typename S, class TensorType>
// void SerializeIfElse(S &serializer, std::shared_ptr<SaveableParamsInterface> const &nsp);
//
// template <typename S, class TensorType>
// std::shared_ptr<SaveableParamsInterface> DeserializeIfElse(S &serializer);

// template <typename S, class SP>
// std::shared_ptr<SaveableParamsInterface> DeserializeHelper(S &serializer)
//{
//  auto nsp_ptr = std::make_shared<SP>();
//  serializer >> *nsp_ptr;
//  return nsp_ptr;
//}

//
// template <typename S, class TensorType>
// void SerializeIfElse(S &serializer, std::shared_ptr<SaveableParamsInterface> const &nsp)
//{
//  std::string next_sp_descriptor = nsp->GetDescription();
//  serializer << next_sp_descriptor;
//
//  if (next_sp_descriptor == SaveableParams::sp_descriptor)
//  {
//    SerializeHelper<S, SaveableParamsInterface>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == PlaceholderSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, PlaceholderSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == WeightsSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, WeightsSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == DropoutSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, DropoutSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == LeakyReluSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, LeakyReluSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == RandomisedReluSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, RandomisedReluSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == SoftmaxSaveableParams::sp_descriptor)
//  {
//    SerializeHelper<S, SoftmaxSaveableParamsInterface>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == Convolution1DSaveableParams::sp_descriptor)
//  {
//    SerializeHelper<S, Convolution1DSaveableParamsInterface>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == MaxPoolSaveableParams::sp_descriptor)
//  {
//    SerializeHelper<S, MaxPoolSaveableParamsInterface>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == TransposeSaveableParams::sp_descriptor)
//  {
//    SerializeHelper<S, TransposeSaveableParamsInterface>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == ReshapeSaveableParams::sp_descriptor)
//  {
//    SerializeHelper<S, ReshapeSaveableParamsInterface>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == FullyConnectedSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, FullyConnectedSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else if (next_sp_descriptor == ConvolutionLayerSaveableParams<TensorType>::sp_descriptor)
//  {
//    SerializeHelper<S, ConvolutionLayerSaveableParams<TensorType>>(serializer, nsp);
//  }
//  else
//  {
//    throw std::runtime_error("Unknown type for Serialization");
//  }
//}
//
// template <typename S, class TensorType>
// std::shared_ptr<SaveableParamsInterface> DeserializeIfElse(S &serializer)
//{
//  std::string next_sp_descriptor;
//  serializer >> next_sp_descriptor;
//  //      std::cout << "next_sp_descriptor: " << next_sp_descriptor << std::endl;
//
//  std::shared_ptr<SaveableParamsInterface> nsp_ptr;
//  if (next_sp_descriptor == SaveableParams::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, SaveableParamsInterface>(serializer);
//  }
//  else if (next_sp_descriptor == PlaceholderSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, PlaceholderSaveableParams<TensorType>>(serializer);
//  }
//  else if (next_sp_descriptor == WeightsSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, WeightsSaveableParams<TensorType>>(serializer);
//  }
//  else if (next_sp_descriptor == DropoutSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, DropoutSaveableParams<TensorType>>(serializer);
//  }
//  else if (next_sp_descriptor == LeakyReluSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, LeakyReluSaveableParams<TensorType>>(serializer);
//  }
//  else if (next_sp_descriptor == RandomisedReluSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, RandomisedReluSaveableParams<TensorType>>(serializer);
//  }
//  else if (next_sp_descriptor == SoftmaxSaveableParams::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, SoftmaxSaveableParamsInterface>(serializer);
//  }
//  else if (next_sp_descriptor == Convolution1DSaveableParams::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, Convolution1DSaveableParamsInterface>(serializer);
//  }
//  else if (next_sp_descriptor == MaxPoolSaveableParams::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, MaxPoolSaveableParamsInterface>(serializer);
//  }
//  else if (next_sp_descriptor == TransposeSaveableParams::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, TransposeSaveableParamsInterface>(serializer);
//  }
//  else if (next_sp_descriptor == ReshapeSaveableParams::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, ReshapeSaveableParamsInterface>(serializer);
//  }
//  else if (next_sp_descriptor == FullyConnectedSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, FullyConnectedSaveableParams<TensorType>>(serializer);
//  }
//  else if (next_sp_descriptor == ConvolutionLayerSaveableParams<TensorType>::sp_descriptor)
//  {
//    nsp_ptr = DeserializeHelper<S, ConvolutionLayerSaveableParams<TensorType>>(serializer);
//  }
//  else
//  {
//    throw std::runtime_error("Unknown type for deserialization");
//  }
//  return nsp_ptr;
//}

}  // namespace ml
}  // namespace fetch
