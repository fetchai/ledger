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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"
#include "ml/optimisation/learning_rate_params.hpp"
#include "ml/regularisers/reg_types.hpp"
#include "ml/saveparams/saveable_params.hpp"
#include "ml/utilities/graph_builder.hpp"

namespace fetch {
namespace ml {

namespace model {
template <typename TensorType>
class Model;
}

namespace dataloaders {
template <typename TensorType>
class TensorDataLoader;
}

namespace optimisers {
template <typename TensorType>
class SGDOptimiser;

template <typename TensorType>
class AdamOptimiser;
}  // namespace optimisers

namespace utilities {

template <typename TensorType>
class MinMaxScaler;
}

}  // namespace ml

namespace serializers {

///////////////////////////////////////////
/// OP SPECIFIC SERIALISATION FUNCTIONS ///
///////////////////////////////////////////

template <class TensorType, typename D, class SP, typename MapType>
void SerializeImplementation(MapType &map, uint8_t code,
                             std::shared_ptr<fetch::ml::OpsSaveableParams> op)
{
  auto castnode = std::dynamic_pointer_cast<SP>(op);
  map.Append(code, *castnode);
}

template <class TensorType, typename D, class SP, typename MapType>
std::shared_ptr<SP> DeserializeImplementation(MapType &map, uint8_t code)
{
  // deserialize concrete op type
  auto sp_ptr = std::make_shared<SP>();
  map.ExpectKeyGetValue(code, *sp_ptr);
  return sp_ptr;
}

template <class TensorType, typename D, typename MapType>
void SerializeAnyOp(MapType &map, uint8_t code, fetch::ml::OpType const &op_type,
                    std::shared_ptr<fetch::ml::OpsSaveableParams> op)
{
  switch (op_type)
  {
  case ml::OpType::OP_ABS:
  {
    SerializeImplementation<TensorType, D, ml::OpAbsSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_ADD:
  {
    SerializeImplementation<TensorType, D, ml::OpAddSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_CONCATENATE:
  {
    SerializeImplementation<TensorType, D, ml::OpConcatenateSaveableParams<TensorType>>(map, code,
                                                                                        op);
    break;
  }
  case ml::OpType::OP_CONSTANT:
  {
    SerializeImplementation<TensorType, D, ml::OpConstantSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_CONVOLUTION_1D:
  {
    SerializeImplementation<TensorType, D, ml::OpConvolution1DSaveableParams<TensorType>>(map, code,
                                                                                          op);
    break;
  }
  case ml::OpType::OP_CONVOLUTION_2D:
  {
    SerializeImplementation<TensorType, D, ml::OpConvolution2DSaveableParams<TensorType>>(map, code,
                                                                                          op);
    break;
  }
  case ml::OpType::LOSS_CROSS_ENTROPY:
  {
    SerializeImplementation<TensorType, D, ml::OpCrossEntropyLossSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::OP_DATAHOLDER:
  {
    SerializeImplementation<TensorType, D, ml::OpDataHolderSaveableParams<TensorType>>(map, code,
                                                                                       op);
    break;
  }
  case ml::OpType::OP_DIVIDE:
  {
    SerializeImplementation<TensorType, D, ml::OpDivideSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_DROPOUT:
  {
    SerializeImplementation<TensorType, D, ml::OpDropoutSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_ELU:
  {
    SerializeImplementation<TensorType, D, ml::OpEluSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_GELU:
  {
    SerializeImplementation<TensorType, D, ml::OpGeluSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_EMBEDDINGS:
  {
    SerializeImplementation<TensorType, D, ml::OpEmbeddingsSaveableParams<TensorType>>(map, code,
                                                                                       op);
    break;
  }
  case ml::OpType::OP_EXP:
  {
    SerializeImplementation<TensorType, D, ml::OpExpSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_FLATTEN:
  {
    SerializeImplementation<TensorType, D, ml::OpFlattenSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_LAYER_NORM:
  {
    SerializeImplementation<TensorType, D, ml::OpLayerNormSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_LEAKY_RELU:
  {
    SerializeImplementation<TensorType, D, ml::OpLeakyReluSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_PRELU_OP:
  {
    SerializeImplementation<TensorType, D, ml::OpPReluOpSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_LOG:
  {
    SerializeImplementation<TensorType, D, ml::OpLogSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_LOGSIGMOID:
  {
    SerializeImplementation<TensorType, D, ml::OpLogSigmoidSaveableParams<TensorType>>(map, code,
                                                                                       op);
    break;
  }
  case ml::OpType::OP_LOGSOFTMAX:
  {
    SerializeImplementation<TensorType, D, ml::OpLogSoftmaxSaveableParams<TensorType>>(map, code,
                                                                                       op);
    break;
  }
  case ml::OpType::OP_MATRIX_MULTIPLY:
  {
    SerializeImplementation<TensorType, D, ml::OpMatrixMultiplySaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LOSS_MEAN_SQUARE_ERROR:
  {
    SerializeImplementation<TensorType, D, ml::OpMeanSquareErrorSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::OP_MASK_FILL:
  {
    SerializeImplementation<TensorType, D, ml::OpMaskFillSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_MAX_POOL_1D:
  {
    SerializeImplementation<TensorType, D, ml::OpMaxPool1DSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_MAX_POOL_2D:
  {
    SerializeImplementation<TensorType, D, ml::OpMaxPool2DSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_MAX_POOL:
  {
    SerializeImplementation<TensorType, D, ml::OpMaxPoolSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_AVG_POOL_1D:
  {
    SerializeImplementation<TensorType, D, ml::OpAvgPool1DSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_AVG_POOL_2D:
  {
    SerializeImplementation<TensorType, D, ml::OpAvgPool2DSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_MAXIMUM:
  {
    SerializeImplementation<TensorType, D, ml::OpMaximumSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_MULTIPLY:
  {
    SerializeImplementation<TensorType, D, ml::OpMultiplySaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_PLACEHOLDER:
  {
    SerializeImplementation<TensorType, D, ml::OpPlaceholderSaveableParams<TensorType>>(map, code,
                                                                                        op);
    break;
  }
  case ml::OpType::OP_RANDOMISED_RELU:
  {
    SerializeImplementation<TensorType, D, ml::OpRandomisedReluSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::OP_RELU:
  {
    SerializeImplementation<TensorType, D, ml::OpReluSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_RESHAPE:
  {
    SerializeImplementation<TensorType, D, ml::OpReshapeSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_SIGMOID:
  {
    SerializeImplementation<TensorType, D, ml::OpSigmoidSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_SOFTMAX:
  {
    SerializeImplementation<TensorType, D, ml::OpSoftmaxSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_SLICE:
  {
    SerializeImplementation<TensorType, D, ml::OpSliceSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_STRIDED_SLICE:
  {
    SerializeImplementation<TensorType, D, ml::OpStridedSliceSaveableParams<TensorType>>(map, code,
                                                                                         op);
    break;
  }
  case ml::OpType::OP_REDUCE_MEAN:
  {
    SerializeImplementation<TensorType, D, ml::OpReduceMeanSaveableParams<TensorType>>(map, code,
                                                                                       op);
    break;
  }
  case ml::OpType::LOSS_SOFTMAX_CROSS_ENTROPY:
  {
    SerializeImplementation<TensorType, D, ml::OpSoftmaxCrossEntropySaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::OP_SQRT:
  {
    SerializeImplementation<TensorType, D, ml::OpSQRTSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_SUBTRACT:
  {
    SerializeImplementation<TensorType, D, ml::OpSubtractSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_SWITCH:
  {
    SerializeImplementation<TensorType, D, ml::OpSwitchSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_TANH:
  {
    SerializeImplementation<TensorType, D, ml::OpTanhSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_TRANSPOSE:
  {
    SerializeImplementation<TensorType, D, ml::OpTransposeSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::OP_ONE_HOT:
  {
    SerializeImplementation<TensorType, D, ml::OpOneHotSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_TOP_K:
  {
    SerializeImplementation<TensorType, D, ml::OpTopKSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_SQUEEZE:
  {
    SerializeImplementation<TensorType, D, ml::OpSqueezeSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_VARIABLE:
  {
    SerializeImplementation<TensorType, D, ml::OpVariableSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::OP_WEIGHTS:
  {
    SerializeImplementation<TensorType, D, ml::OpWeightsSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_1D:
  {
    SerializeImplementation<TensorType, D, ml::LayerConvolution1DSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_2D:
  {
    SerializeImplementation<TensorType, D, ml::LayerConvolution2DSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_FULLY_CONNECTED:
  {
    SerializeImplementation<TensorType, D, ml::LayerFullyConnectedSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_LAYER_NORM:
  {
    SerializeImplementation<TensorType, D, ml::LayerLayerNormSaveableParams<TensorType>>(map, code,
                                                                                         op);
    break;
  }
  case ml::OpType::LAYER_MULTI_HEAD_ATTENTION:
  {
    SerializeImplementation<TensorType, D, ml::LayerMultiHeadSaveableParams<TensorType>>(map, code,
                                                                                         op);
    break;
  }
  case ml::OpType::LAYER_PRELU:
  {
    SerializeImplementation<TensorType, D, ml::LayerPReluSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION:
  {
    SerializeImplementation<TensorType, D,
                            ml::LayerScaledDotProductAttentionSaveableParams<TensorType>>(map, code,
                                                                                          op);
    break;
  }
  case ml::OpType::LAYER_SELF_ATTENTION_ENCODER:
  {
    SerializeImplementation<TensorType, D, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_SKIP_GRAM:
  {
    SerializeImplementation<TensorType, D, ml::LayerSkipGramSaveableParams<TensorType>>(map, code,
                                                                                        op);
    break;
  }
  case ml::OpType::METRIC_CATEGORICAL_ACCURACY:
  {
    SerializeImplementation<TensorType, D, ml::OpCategoricalAccuracySaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::GRAPH:
  case ml::OpType::SUBGRAPH:
  {
    throw ml::exceptions::InvalidMode(
        "Graph and subgraph shouldn't be serialized with SerializeImplementation");
  }
  case ml::OpType::NONE:
  default:
  {
    throw ml::exceptions::InvalidMode("Unknown type for Serialization");
  }
  }
}

template <class TensorType, typename D, typename MapType>
void DeserializeAnyOp(MapType &map, uint8_t code, fetch::ml::OpType const &op_type,
                      std::shared_ptr<fetch::ml::OpsSaveableParams> &op)
{
  switch (op_type)
  {
  case ml::OpType::OP_ABS:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpAbsSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_ADD:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpAddSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_CONCATENATE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpConcatenateSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_CONSTANT:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpConstantSaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::OP_CONVOLUTION_1D:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpConvolution1DSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_CONVOLUTION_2D:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpConvolution2DSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LOSS_CROSS_ENTROPY:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpCrossEntropyLossSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_DATAHOLDER:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpDataHolderSaveableParams<TensorType>>(map,
                                                                                              code);
    break;
  }
  case ml::OpType::OP_DIVIDE:
  {
    op =
        DeserializeImplementation<TensorType, D, ml::OpDivideSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_DROPOUT:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpDropoutSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_ELU:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpEluSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_GELU:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpGeluSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_EMBEDDINGS:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpEmbeddingsSaveableParams<TensorType>>(map,
                                                                                              code);
    break;
  }
  case ml::OpType::OP_EXP:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpExpSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_FLATTEN:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpFlattenSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_LAYER_NORM:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpLayerNormSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_LEAKY_RELU:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpLeakyReluSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_PRELU_OP:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpPReluOpSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_LOG:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpLogSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_LOGSIGMOID:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpLogSigmoidSaveableParams<TensorType>>(map,
                                                                                              code);
    break;
  }
  case ml::OpType::OP_LOGSOFTMAX:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpLogSoftmaxSaveableParams<TensorType>>(map,
                                                                                              code);
    break;
  }
  case ml::OpType::OP_MASK_FILL:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMaskFillSaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::OP_MATRIX_MULTIPLY:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMatrixMultiplySaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LOSS_MEAN_SQUARE_ERROR:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMeanSquareErrorSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_MAX_POOL_1D:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMaxPool1DSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_MAX_POOL_2D:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMaxPool2DSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_MAX_POOL:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMaxPoolSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_AVG_POOL_1D:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpAvgPool1DSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_AVG_POOL_2D:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpAvgPool2DSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_MAXIMUM:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMaximumSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_MULTIPLY:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMultiplySaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::OP_PLACEHOLDER:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpPlaceholderSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_RANDOMISED_RELU:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpRandomisedReluSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_RELU:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpReluSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_RESHAPE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpReshapeSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_SIGMOID:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpSigmoidSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_SOFTMAX:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpSoftmaxSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::LOSS_SOFTMAX_CROSS_ENTROPY:
  {
    op = DeserializeImplementation<TensorType, D,
                                   ml::OpSoftmaxCrossEntropySaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_SQRT:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpSQRTSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_SUBTRACT:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpSubtractSaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::OP_SWITCH:
  {
    op =
        DeserializeImplementation<TensorType, D, ml::OpSwitchSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_SLICE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpSliceSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_STRIDED_SLICE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpStridedSliceSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_REDUCE_MEAN:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpReduceMeanSaveableParams<TensorType>>(map,
                                                                                              code);
    break;
  }
  case ml::OpType::OP_TANH:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpTanhSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_TRANSPOSE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpTransposeSaveableParams<TensorType>>(map,
                                                                                             code);
    break;
  }
  case ml::OpType::OP_ONE_HOT:
  {
    op =
        DeserializeImplementation<TensorType, D, ml::OpOneHotSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_TOP_K:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpTopKSaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::OP_SQUEEZE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpSqueezeSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::OP_VARIABLE:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpVariableSaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::OP_WEIGHTS:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpWeightsSaveableParams<TensorType>>(map,
                                                                                           code);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_1D:
  {
    op = DeserializeImplementation<TensorType, D, ml::LayerConvolution1DSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_2D:
  {
    op = DeserializeImplementation<TensorType, D, ml::LayerConvolution2DSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LAYER_FULLY_CONNECTED:
  {
    op =
        DeserializeImplementation<TensorType, D, ml::LayerFullyConnectedSaveableParams<TensorType>>(
            map, code);
    break;
  }
  case ml::OpType::LAYER_LAYER_NORM:
  {
    op = DeserializeImplementation<TensorType, D, ml::LayerLayerNormSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LAYER_MULTI_HEAD_ATTENTION:
  {
    op = DeserializeImplementation<TensorType, D, ml::LayerMultiHeadSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LAYER_PRELU:
  {
    op = DeserializeImplementation<TensorType, D, ml::LayerPReluSaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION:
  {
    op = DeserializeImplementation<TensorType, D,
                                   ml::LayerScaledDotProductAttentionSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::LAYER_SELF_ATTENTION_ENCODER:
  {
    op = DeserializeImplementation<TensorType, D,
                                   ml::LayerSelfAttentionEncoderSaveableParams<TensorType>>(map,
                                                                                            code);
    break;
  }
  case ml::OpType::LAYER_SKIP_GRAM:
  {
    op = DeserializeImplementation<TensorType, D, ml::LayerSkipGramSaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::METRIC_CATEGORICAL_ACCURACY:
  {

    op = DeserializeImplementation<TensorType, D,
                                   ml::OpCategoricalAccuracySaveableParams<TensorType>>(map, code);
    break;
  }
  case ml::OpType::GRAPH:
  case ml::OpType::SUBGRAPH:
  {
    throw ml::exceptions::InvalidMode(
        "Graph and Subgraph shouldn't be deserialized with DeserializeImplementation");
    break;
  }
  case ml::OpType::NONE:
  default:
  {
    throw ml::exceptions::InvalidMode("Unknown type for Deserialization");
  }
  }
}

/**
 * serializer for OpType
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<ml::OpsSaveableParams, D>
{
  using Type       = ml::OpsSaveableParams;
  using DriverType = D;

  static uint8_t constexpr OP_CODE            = 1;
  static uint8_t constexpr IS_TRAINING        = 2;
  static uint8_t constexpr BATCH_INPUT_SHAPES = 3;
  static uint8_t constexpr BATCH_OUTPUT_SHAPE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &osp)
  {
    auto map = map_constructor(BATCH_OUTPUT_SHAPE);
    map.Append(OP_CODE, osp.op_type);
    map.Append(IS_TRAINING, osp.is_training);
    map.Append(BATCH_INPUT_SHAPES, osp.batch_input_shapes);
    map.Append(BATCH_OUTPUT_SHAPE, osp.batch_output_shape);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &osp)
  {
    map.ExpectKeyGetValue(OP_CODE, osp.op_type);
    map.ExpectKeyGetValue(IS_TRAINING, osp.is_training);
    map.ExpectKeyGetValue(BATCH_INPUT_SHAPES, osp.batch_input_shapes);
    map.ExpectKeyGetValue(BATCH_OUTPUT_SHAPE, osp.batch_output_shape);
  }
};

/**
 * serializer for OpType
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<ml::OpType, D>
{
  using Type       = ml::OpType;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &body)
  {
    auto map      = map_constructor(1);
    auto enum_val = static_cast<uint16_t>(body);
    map.Append(OP_CODE, enum_val);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &body)
  {
    uint16_t op_code_int = 0;
    map.ExpectKeyGetValue(OP_CODE, op_code_int);

    body = static_cast<Type>(op_code_int);
  }
};

/**
 * serializer for GraphSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::GraphSaveableParams<TensorType>, D>
{
  using Type       = ml::GraphSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE            = 1;
  static uint8_t const CONNECTIONS_FIRST  = 2;
  static uint8_t const CONNECTIONS_SECOND = 3;
  static uint8_t const NODES              = 4;
  static uint8_t const GRAPH_STATE        = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);
    map.Append(OP_CODE, sp.op_type);

    // split connections into keys and values
    std::vector<std::string>              connections_first;
    std::vector<std::vector<std::string>> connections_second;

    std::transform(begin(sp.connections), end(sp.connections),
                   std::back_inserter(connections_first),
                   [](auto const &pair) { return pair.first; });

    std::transform(begin(sp.connections), end(sp.connections),
                   std::back_inserter(connections_second),
                   [](auto const &pair) { return pair.second; });

    map.Append(CONNECTIONS_FIRST, connections_first);
    map.Append(CONNECTIONS_SECOND, connections_second);

    std::vector<ml::NodeSaveableParams<TensorType>> nodevec;
    for (auto const &node_name : connections_first)
    {
      auto nsp =
          std::dynamic_pointer_cast<ml::NodeSaveableParams<TensorType>>(sp.nodes.at(node_name));
      nodevec.emplace_back(*nsp);
    }

    map.Append(NODES, nodevec);
    map.Append(GRAPH_STATE, sp.graph_state);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    std::vector<std::string>              connections_first;
    std::vector<std::vector<std::string>> connections_second;

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(CONNECTIONS_FIRST, connections_first);
    map.ExpectKeyGetValue(CONNECTIONS_SECOND, connections_second);

    auto it1 = connections_first.begin();
    for (auto const &it2 : connections_second)
    {
      sp.connections.emplace_back(std::make_pair(*it1, it2));
      ++it1;
    }

    std::vector<ml::NodeSaveableParams<TensorType>> nodevec;
    map.ExpectKeyGetValue(NODES, nodevec);

    auto it3 = nodevec.begin();
    for (auto const &node_name : connections_first)
    {
      auto nsp = std::make_shared<ml::NodeSaveableParams<TensorType>>(*it3);
      sp.nodes.insert(std::make_pair(node_name, nsp));
      ++it3;
    }

    map.ExpectKeyGetValue(GRAPH_STATE, sp.graph_state);
  }
};

/**
 * serializer for SubGraphSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::SubGraphSaveableParams<TensorType>, D>
{
  using Type = ml::SubGraphSaveableParams<TensorType>;

  using DriverType                      = D;
  static uint8_t const GRAPH            = 1;
  static uint8_t const BASE_OPS         = 2;
  static uint8_t const OP_CODE          = 3;
  static uint8_t const INPUT_NODE_NAMES = 4;
  static uint8_t const OUTPUT_NODE_NAME = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);

    // serialize parent class first
    auto base_pointer = static_cast<ml::GraphSaveableParams<TensorType> const *>(&sp);
    map.Append(GRAPH, *(base_pointer));

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(INPUT_NODE_NAMES, sp.input_node_names);
    map.Append(OUTPUT_NODE_NAME, sp.output_node_name);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::GraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(GRAPH, (*base_pointer));

    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(INPUT_NODE_NAMES, sp.input_node_names);
    map.ExpectKeyGetValue(OUTPUT_NODE_NAME, sp.output_node_name);
  }
};

/**
 * serializer for NodeSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::NodeSaveableParams<TensorType>, D>
{
  using Type       = ml::NodeSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const NAME    = 1;
  static uint8_t const OP_CODE = 2;
  static uint8_t const OP      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    map.Append(NAME, sp.name);
    map.Append(OP_CODE, sp.operation_type);

    SerializeAnyOp<TensorType, D>(map, OP, sp.operation_type, sp.op_save_params);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(NAME, sp.name);
    map.ExpectKeyGetValue(OP_CODE, sp.operation_type);

    DeserializeAnyOp<TensorType, D>(map, OP, sp.operation_type, sp.op_save_params);
  }
};

/**
 * serializer for abs saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAbsSaveableParams<TensorType>, D>
{
  using Type       = ml::OpAbsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const BASE_OPS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for add saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAddSaveableParams<TensorType>, D>
{
  using Type       = ml::OpAddSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const AXES     = 2;
  static uint8_t const BASE_OPS = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);
    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXES, sp.axes);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXES, sp.axes);
  }
};

/**
 * serializer for concatenate saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConcatenateSaveableParams<TensorType>, D>
{
  using Type       = ml::OpConcatenateSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const AXIS     = 2;
  static uint8_t const BASE_OPS = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXIS, sp.axis);
  }
};

/**
 * serializer for OpConstantSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConstantSaveableParams<TensorType>, D>
{
  using Type       = ml::OpConstantSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for conv1d saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConvolution1DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpConvolution1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const STRIDE_SIZE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for conv2d saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConvolution2DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpConvolution2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const STRIDE_SIZE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for OpCrossEntropyLossSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpCrossEntropyLossSaveableParams<TensorType>, D>
{
  using Type       = ml::OpCrossEntropyLossSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const BASE_OPS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
  }
};

/**
 * serializer for OpDataHolderSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpDataHolderSaveableParams<TensorType>, D>
{
  using Type       = ml::OpDataHolderSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  static uint8_t const DATA              = 3;
  static uint8_t const DATA_PRESENT      = 4;
  static uint8_t const FUTURE_DATA_SHAPE = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);

    if (sp.data)
    {
      map.Append(DATA_PRESENT, true);
      map.Append(DATA, *(sp.data));
    }
    else
    {
      map.Append(DATA_PRESENT, false);
    }

    map.Append(FUTURE_DATA_SHAPE, sp.future_data_shape);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    bool has_data = true;
    map.ExpectKeyGetValue(DATA_PRESENT, has_data);
    if (has_data)
    {
      TensorType data;
      map.ExpectKeyGetValue(DATA, data);
      sp.data = std::make_shared<TensorType>(data);
    }

    map.ExpectKeyGetValue(FUTURE_DATA_SHAPE, sp.future_data_shape);
  }
};

/**
 * serializer for divide saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpDivideSaveableParams<TensorType>, D>
{
  using Type       = ml::OpDivideSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const BASE_OPS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    map.Append(OP_CODE, sp.op_type);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
  }
};

/**
 * serializer for divide saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpDropoutSaveableParams<TensorType>, D>
{
  using Type       = ml::OpDropoutSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const RANDOM_SEED = 3;
  static uint8_t const PROBABILITY = 4;
  static uint8_t const BUFFER      = 5;
  static uint8_t const INDEX       = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);
    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(RANDOM_SEED, sp.random_seed);
    map.Append(PROBABILITY, sp.probability);
    map.Append(BUFFER, sp.buffer);
    map.Append(INDEX, sp.index);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(RANDOM_SEED, sp.random_seed);
    map.ExpectKeyGetValue(PROBABILITY, sp.probability);
    map.ExpectKeyGetValue(BUFFER, sp.buffer);
    map.ExpectKeyGetValue(INDEX, sp.index);
  }
};

/**
 * serializer for Elu saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpEluSaveableParams<TensorType>, D>
{
  using Type       = ml::OpEluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const VALUE    = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(VALUE, sp.a);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(VALUE, sp.a);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpEmbeddingsSaveableParams<TensorType>, D>
{
  using Type       = ml::OpEmbeddingsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE    = 1;
  static uint8_t const BASE_CLASS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);

    // serialize parent class
    auto base_pointer = static_cast<ml::OpWeightsSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    auto base_pointer = static_cast<ml::OpWeightsSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_CLASS, *base_pointer);
  }
};

/**
 * serializer for Exp saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpExpSaveableParams<TensorType>, D>
{
  using Type       = ml::OpExpSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for Flatten saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpFlattenSaveableParams<TensorType>, D>
{
  using Type       = ml::OpFlattenSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const INPUT_SHAPE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(INPUT_SHAPE, sp.input_shape);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(INPUT_SHAPE, sp.input_shape);
  }
};

/**
 * serializer for Elu saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpGeluSaveableParams<TensorType>, D>
{
  using Type       = ml::OpGeluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpLayerNormSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLayerNormSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLayerNormSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const EPSILON  = 3;
  static uint8_t const AXIS     = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(EPSILON, sp.epsilon);
    map.Append(AXIS, sp.axis);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon);
    map.ExpectKeyGetValue(AXIS, sp.axis);
  }
};

/**
 * serializer for LeakyRelu saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLeakyReluSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLeakyReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const VAL      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(VAL, sp.a);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(VAL, sp.a);
  }
};

/**
 * serializer for PReluOp saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpPReluOpSaveableParams<TensorType>, D>
{
  using Type       = ml::OpPReluOpSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const VAL      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(VAL, sp.a);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(VAL, sp.a);
  }
};

/**
 * serializer for Log saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLogSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSigmoidSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLogSigmoidSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSoftmaxSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLogSoftmaxSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const AXIS     = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXIS, sp.axis);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaskFillSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaskFillSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS   = 1;
  static uint8_t const OP_CODE    = 2;
  static uint8_t const FILL_VALUE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(FILL_VALUE, sp.fill_value);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(FILL_VALUE, sp.fill_value);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMatrixMultiplySaveableParams<TensorType>, D>
{
  using Type       = ml::OpMatrixMultiplySaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const TRANSPOSE_A = 3;
  static uint8_t const TRANSPOSE_B = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(TRANSPOSE_A, sp.transpose_a);
    map.Append(TRANSPOSE_B, sp.transpose_b);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(TRANSPOSE_A, sp.transpose_a);
    map.ExpectKeyGetValue(TRANSPOSE_B, sp.transpose_b);
  }
};

/**
 * serializer for MaxPool1D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPool1DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaxPool1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const KERNEL_SIZE = 3;
  static uint8_t const STRIDE_SIZE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for MaxPool saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPoolSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaxPoolSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const KERNEL_SIZE = 3;
  static uint8_t const STRIDE_SIZE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for MaxPool2D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPool2DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaxPool2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const KERNEL_SIZE = 3;
  static uint8_t const STRIDE_SIZE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for AvgPool1D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAvgPool1DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpAvgPool1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const KERNEL_SIZE = 3;
  static uint8_t const STRIDE_SIZE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for Average Pool 2D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAvgPool2DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpAvgPool2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS    = 1;
  static uint8_t const OP_CODE     = 2;
  static uint8_t const KERNEL_SIZE = 3;
  static uint8_t const STRIDE_SIZE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for Mean Square Error saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMeanSquareErrorSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMeanSquareErrorSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS   = 1;
  static uint8_t const OP_CODE    = 2;
  static uint8_t const WEIGHTINGS = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(WEIGHTINGS, sp.weightings);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(WEIGHTINGS, sp.weightings);
  }
};

/**
 * serializer for CategoricalAccuracy saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpCategoricalAccuracySaveableParams<TensorType>, D>
{
  using Type       = ml::OpCategoricalAccuracySaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS   = 1;
  static uint8_t const OP_CODE    = 2;
  static uint8_t const WEIGHTINGS = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(WEIGHTINGS, sp.weightings);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(WEIGHTINGS, sp.weightings);
  }
};

/**
 * serializer for OpMaximumSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaximumSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaximumSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpMultiplySaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMultiplySaveableParams<TensorType>, D>
{
  using Type       = ml::OpMultiplySaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpPlaceholderSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpPlaceholderSaveableParams<TensorType>, D>
{
  using Type       = ml::OpPlaceholderSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpRandomisedReluSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpRandomisedReluSaveableParams<TensorType>, D>
{
  using Type       = ml::OpRandomisedReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS     = 1;
  static uint8_t const OP_CODE      = 2;
  static uint8_t const LOWER_BOUND  = 3;
  static uint8_t const UPPER_BOUND  = 4;
  static uint8_t const RANDOM_SEED  = 5;
  static uint8_t const BUFFER       = 6;
  static uint8_t const INDEX        = 7;
  static uint8_t const RANDOM_VALUE = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(8);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(LOWER_BOUND, sp.lower_bound);
    map.Append(UPPER_BOUND, sp.upper_bound);
    map.Append(RANDOM_SEED, sp.random_seed);
    map.Append(BUFFER, sp.buffer);
    map.Append(INDEX, sp.index);
    map.Append(RANDOM_VALUE, sp.random_value);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(LOWER_BOUND, sp.lower_bound);
    map.ExpectKeyGetValue(UPPER_BOUND, sp.upper_bound);
    map.ExpectKeyGetValue(RANDOM_SEED, sp.random_seed);
    map.ExpectKeyGetValue(BUFFER, sp.buffer);
    map.ExpectKeyGetValue(INDEX, sp.index);
    map.ExpectKeyGetValue(RANDOM_VALUE, sp.random_value);
  }
};

/**
 * serializer for OpReluSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpReluSaveableParams<TensorType>, D>
{
  using Type       = ml::OpReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const BASE_OPS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpReshapeSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpReshapeSaveableParams<TensorType>, D>
{
  using Type       = ml::OpReshapeSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS  = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const NEW_SHAPE = 3;
  static uint8_t const NEW_SIZE  = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(NEW_SHAPE, sp.new_shape);
    map.Append(NEW_SIZE, sp.new_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(NEW_SHAPE, sp.new_shape);
    map.ExpectKeyGetValue(NEW_SIZE, sp.new_size);
  }
};

/**
 * serializer for OpSigmoidSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSigmoidSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSigmoidSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpSliceaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSliceSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSliceSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  static uint8_t const AXES            = 3;
  static uint8_t const INDICES         = 4;
  static uint8_t const AXIS            = 5;
  static uint8_t const INDEX           = 6;
  static uint8_t const SLICE_TYPE      = 7;
  static uint8_t const START_END_SLICE = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(8);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXES, sp.axes);
    map.Append(INDICES, sp.indices);
    map.Append(AXIS, sp.axis);
    map.Append(INDEX, sp.index);
    map.Append(SLICE_TYPE, sp.slice_type);
    map.Append(START_END_SLICE, sp.start_end_slice);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXES, sp.axes);
    map.ExpectKeyGetValue(INDICES, sp.indices);
    map.ExpectKeyGetValue(AXIS, sp.axis);
    map.ExpectKeyGetValue(INDEX, sp.index);
    map.ExpectKeyGetValue(SLICE_TYPE, sp.slice_type);
    map.ExpectKeyGetValue(START_END_SLICE, sp.start_end_slice);
  }
};

/**
 * serializer for OpStridedSliceaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpStridedSliceSaveableParams<TensorType>, D>
{
  using Type       = ml::OpStridedSliceSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  static uint8_t const BEGINS  = 3;
  static uint8_t const ENDS    = 4;
  static uint8_t const STRIDES = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(BEGINS, sp.begins);
    map.Append(ENDS, sp.ends);
    map.Append(STRIDES, sp.strides);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(BEGINS, sp.begins);
    map.ExpectKeyGetValue(ENDS, sp.ends);
    map.ExpectKeyGetValue(STRIDES, sp.strides);
  }
};

/**
 * serializer for OpSqueezeaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSqueezeSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSqueezeSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpReduceMeanaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpReduceMeanSaveableParams<TensorType>, D>
{
  using Type       = ml::OpReduceMeanSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const AXIS     = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXIS, sp.axis);
  }
};

/**
 * serializer for OpSoftmaxSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSoftmaxSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSoftmaxSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const AXIS     = 3;
  static uint8_t const AXES     = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
    map.Append(AXES, sp.axes);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXIS, sp.axis);
    map.ExpectKeyGetValue(AXES, sp.axes);
  }
};

/**
 * serializer for OpSoftmaxCrossEntropySaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSoftmaxCrossEntropySaveableParams<TensorType>, D>
{
  using Type       = ml::OpSoftmaxCrossEntropySaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpSwitchSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSwitchSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSwitchSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpSQRTSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSQRTSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSQRTSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {

    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpSubtractSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSubtractSaveableParams<TensorType>, D>
{
  using Type       = ml::OpSubtractSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpTanhSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpTanhSaveableParams<TensorType>, D>
{
  using Type       = ml::OpTanhSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpTransposeSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpTransposeSaveableParams<TensorType>, D>
{
  using Type       = ml::OpTransposeSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS         = 1;
  static uint8_t const OP_CODE          = 2;
  static uint8_t const TRANSPOSE_VECTOR = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(TRANSPOSE_VECTOR, sp.transpose_vector);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(TRANSPOSE_VECTOR, sp.transpose_vector);
  }
};

/**
 * serializer for OpOneHotSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpOneHotSaveableParams<TensorType>, D>
{
  using Type       = ml::OpOneHotSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS  = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const DEPTH     = 3;
  static uint8_t const AXIS      = 4;
  static uint8_t const ON_VALUE  = 5;
  static uint8_t const OFF_VALUE = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(DEPTH, sp.depth);
    map.Append(AXIS, sp.axis);
    map.Append(ON_VALUE, sp.on_value);
    map.Append(OFF_VALUE, sp.off_value);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(DEPTH, sp.depth);
    map.ExpectKeyGetValue(AXIS, sp.axis);
    map.ExpectKeyGetValue(ON_VALUE, sp.on_value);
    map.ExpectKeyGetValue(OFF_VALUE, sp.off_value);
  }
};

/**
 * serializer for OpTopKSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpTopKSaveableParams<TensorType>, D>
{
  using Type       = ml::OpTopKSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const BASE_OPS = 1;
  static uint8_t const OP_CODE  = 2;
  static uint8_t const K        = 3;
  static uint8_t const SORTED   = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpsSaveableParams const *>(&sp);
    map.Append(BASE_OPS, *(ops_pointer));
    map.Append(OP_CODE, sp.op_type);
    map.Append(K, sp.k);
    map.Append(SORTED, sp.sorted);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpsSaveableParams *>(&sp);
    map.ExpectKeyGetValue(BASE_OPS, (*ops_pointer));
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(K, sp.k);
    map.ExpectKeyGetValue(SORTED, sp.sorted);
  }
};

/**
 * serializer for OpVariableSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpVariableSaveableParams<TensorType>, D>
{
  using Type       = ml::OpVariableSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE    = 1;
  static uint8_t const BASE_CLASS = 2;

  static uint8_t const REGULARISATION_TYPE = 3;
  static uint8_t const REGULARISATION_RATE = 4;

  static uint8_t const HAS_GRADIENT          = 5;
  static uint8_t const GRADIENT_ACCUMULATION = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    // serialize parent class first
    auto base_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);

    map.Append(OP_CODE, sp.op_type);

    // first set the regulariser type
    map.Append(REGULARISATION_TYPE, static_cast<uint8_t>(sp.regularisation_type));
    map.Append(REGULARISATION_RATE, sp.regularisation_rate);

    if (sp.gradient_accumulation)
    {
      map.Append(HAS_GRADIENT, true);
      map.Append(GRADIENT_ACCUMULATION, *(sp.gradient_accumulation));
    }
    else
    {
      map.Append(HAS_GRADIENT, false);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_CLASS, *base_pointer);
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    uint8_t rt;
    map.ExpectKeyGetValue(REGULARISATION_TYPE, rt);
    sp.regularisation_type = static_cast<fetch::ml::RegularisationType>(rt);
    map.ExpectKeyGetValue(REGULARISATION_RATE, sp.regularisation_rate);

    bool has_gradient;
    map.ExpectKeyGetValue(HAS_GRADIENT, has_gradient);
    if (has_gradient)
    {
      TensorType ga;
      map.ExpectKeyGetValue(GRADIENT_ACCUMULATION, ga);
      sp.gradient_accumulation = std::make_shared<TensorType>(ga);
    }
  }
};

/**
 * serializer for OpWeightsSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpWeightsSaveableParams<TensorType>, D>
{
  using Type       = ml::OpWeightsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE    = 1;
  static uint8_t const BASE_CLASS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto base_pointer = static_cast<ml::OpVariableSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::OpVariableSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_CLASS, *base_pointer);
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/////////////////////////
/// LAYER SERIALIZERS ///
/////////////////////////

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerConvolution1DSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerConvolution1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH       = 1;
  static uint8_t const OP_CODE         = 2;
  static uint8_t const KERNEL_SIZE     = 3;
  static uint8_t const INPUT_CHANNELS  = 4;
  static uint8_t const OUTPUT_CHANNELS = 5;
  static uint8_t const STRIDE_SIZE     = 6;
  static uint8_t const IS_INITIALISED  = 7;
  static uint8_t const WEIGHTS_NAME    = 8;
  static uint8_t const INIT_MODE       = 9;
  static uint8_t const SEED            = 10;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(10);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OUTPUT_CHANNELS, sp.output_channels);
    map.Append(STRIDE_SIZE, sp.stride_size);
    map.Append(IS_INITIALISED, sp.is_initialised);
    map.Append(WEIGHTS_NAME, sp.weights_name);
    map.Append(INIT_MODE, sp.init_mode);
    map.Append(SEED, sp.seed);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    // deserialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(INPUT_CHANNELS, sp.input_channels);
    map.ExpectKeyGetValue(OUTPUT_CHANNELS, sp.output_channels);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
    map.ExpectKeyGetValue(IS_INITIALISED, sp.is_initialised);
    map.ExpectKeyGetValue(WEIGHTS_NAME, sp.weights_name);
    map.ExpectKeyGetValue(INIT_MODE, sp.init_mode);
    map.ExpectKeyGetValue(SEED, sp.seed);
  }
};

/**
 * serializer for Conv2d layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerConvolution2DSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerConvolution2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH       = 1;
  static uint8_t const OP_CODE         = 2;
  static uint8_t const KERNEL_SIZE     = 3;
  static uint8_t const INPUT_CHANNELS  = 4;
  static uint8_t const OUTPUT_CHANNELS = 5;
  static uint8_t const STRIDE_SIZE     = 6;
  static uint8_t const IS_INITIALISED  = 7;
  static uint8_t const WEIGHTS_NAME    = 8;
  static uint8_t const INIT_MODE       = 9;
  static uint8_t const SEED            = 10;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(10);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OUTPUT_CHANNELS, sp.output_channels);
    map.Append(STRIDE_SIZE, sp.stride_size);
    map.Append(IS_INITIALISED, sp.is_initialised);
    map.Append(WEIGHTS_NAME, sp.weights_name);
    map.Append(INIT_MODE, sp.init_mode);
    map.Append(SEED, sp.seed);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    // deserialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(INPUT_CHANNELS, sp.input_channels);
    map.ExpectKeyGetValue(OUTPUT_CHANNELS, sp.output_channels);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
    map.ExpectKeyGetValue(IS_INITIALISED, sp.is_initialised);
    map.ExpectKeyGetValue(WEIGHTS_NAME, sp.weights_name);
    map.ExpectKeyGetValue(INIT_MODE, sp.init_mode);
    map.ExpectKeyGetValue(SEED, sp.seed);
  }
};

/**
 * serializer for FullyConnectedLayer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerFullyConnectedSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerFullyConnectedSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t constexpr SUB_GRAPH        = 1;
  static uint8_t constexpr OP_CODE          = 2;
  static uint8_t constexpr IN_SIZE          = 3;
  static uint8_t constexpr OUT_SIZE         = 4;
  static uint8_t constexpr TIME_DISTRIBUTED = 5;
  static uint8_t constexpr IS_INITIALISED   = 6;
  static uint8_t constexpr WEIGHTS_NAME     = 7;
  static uint8_t constexpr BIAS_NAME        = 8;
  static uint8_t constexpr INIT_MODE        = 9;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(INIT_MODE);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(IN_SIZE, sp.total_inputs_);
    map.Append(OUT_SIZE, sp.total_outputs_);
    map.Append(TIME_DISTRIBUTED, sp.time_distributed);
    map.Append(IS_INITIALISED, sp.is_initialised);
    map.Append(WEIGHTS_NAME, sp.weights_name);
    map.Append(BIAS_NAME, sp.bias_name);
    map.Append(INIT_MODE, sp.init_mode);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.total_inputs_);
    map.ExpectKeyGetValue(OUT_SIZE, sp.total_outputs_);
    map.ExpectKeyGetValue(TIME_DISTRIBUTED, sp.time_distributed);
    map.ExpectKeyGetValue(IS_INITIALISED, sp.is_initialised);
    map.ExpectKeyGetValue(WEIGHTS_NAME, sp.weights_name);
    map.ExpectKeyGetValue(BIAS_NAME, sp.bias_name);
    map.ExpectKeyGetValue(INIT_MODE, sp.init_mode);
  }
};

/**
 * serializer for OpMaximumSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerLayerNormSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerLayerNormSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH  = 1;
  static uint8_t const OP_CODE    = 2;
  static uint8_t const DATA_SHAPE = 3;
  static uint8_t const AXIS       = 4;
  static uint8_t const EPSILON    = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);
    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(DATA_SHAPE, sp.data_shape);
    map.Append(AXIS, sp.axis);
    map.Append(EPSILON, sp.epsilon);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(DATA_SHAPE, sp.data_shape);
    map.ExpectKeyGetValue(AXIS, sp.axis);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon);
  }
};

/**
 * serializer for LayerMultiHeadSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerMultiHeadSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerMultiHeadSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const VALUE_DIM = 3;
  static uint8_t const KEY_DIM   = 4;
  static uint8_t const N_HEADS   = 5;
  static uint8_t const MODEL_DIM = 6;
  static uint8_t const DROPOUT   = 7;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(7);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(VALUE_DIM, sp.value_dim);
    map.Append(KEY_DIM, sp.key_dim);
    map.Append(N_HEADS, sp.n_heads);
    map.Append(MODEL_DIM, sp.model_dim);
    map.Append(DROPOUT, sp.dropout);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(VALUE_DIM, sp.value_dim);
    map.ExpectKeyGetValue(KEY_DIM, sp.key_dim);
    map.ExpectKeyGetValue(N_HEADS, sp.n_heads);
    map.ExpectKeyGetValue(MODEL_DIM, sp.model_dim);
    map.ExpectKeyGetValue(DROPOUT, sp.dropout);
  }
};

/**
 * serializer for PRELU layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerPReluSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerPReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for LayerScaledDotProductAttentionSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerScaledDotProductAttentionSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerScaledDotProductAttentionSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const KEY_DIM   = 3;
  static uint8_t const DROPOUT   = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KEY_DIM, sp.key_dim);
    map.Append(DROPOUT, sp.dropout);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KEY_DIM, sp.key_dim);
    map.ExpectKeyGetValue(DROPOUT, sp.dropout);
  }
};

/**
 * serializer for self attention layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerSelfAttentionEncoderSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerSelfAttentionEncoderSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH           = 1;
  static uint8_t const OP_CODE             = 2;
  static uint8_t const N_HEADS             = 3;
  static uint8_t const MODEL_DIM           = 4;
  static uint8_t const FF_DIM              = 5;
  static uint8_t const RESIDUAL_DROPOUT    = 6;
  static uint8_t const ATTENTION_DROPOUT   = 7;
  static uint8_t const FEEDFORWARD_DROPOUT = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(8);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(N_HEADS, sp.n_heads);
    map.Append(MODEL_DIM, sp.model_dim);
    map.Append(FF_DIM, sp.ff_dim);
    map.Append(RESIDUAL_DROPOUT, sp.residual_dropout);
    map.Append(ATTENTION_DROPOUT, sp.attention_dropout);
    map.Append(FEEDFORWARD_DROPOUT, sp.feedforward_dropout);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(N_HEADS, sp.n_heads);
    map.ExpectKeyGetValue(MODEL_DIM, sp.model_dim);
    map.ExpectKeyGetValue(FF_DIM, sp.ff_dim);
    map.ExpectKeyGetValue(RESIDUAL_DROPOUT, sp.residual_dropout);
    map.ExpectKeyGetValue(ATTENTION_DROPOUT, sp.attention_dropout);
    map.ExpectKeyGetValue(FEEDFORWARD_DROPOUT, sp.feedforward_dropout);
  }
};

/**
 * serializer for LayerSkipGramSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerSkipGramSaveableParams<TensorType>, D>
{
  using Type       = ml::LayerSkipGramSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const IN_SIZE   = 3;
  static uint8_t const OUT_SIZE  = 4;
  static uint8_t const EMBED_IN  = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(IN_SIZE, sp.in_size);
    map.Append(OUT_SIZE, sp.out_size);
    map.Append(EMBED_IN, sp.embed_in);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.in_size);
    map.ExpectKeyGetValue(OUT_SIZE, sp.out_size);
    map.ExpectKeyGetValue(EMBED_IN, sp.embed_in);
  }
};

/**
 * serializer for Optimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::Optimiser<TensorType>, D>
{
  using Type       = ml::optimisers::Optimiser<TensorType>;
  using DriverType = D;

  // public member variables
  static uint8_t const GRAPH               = 1;
  static uint8_t const INPUT_NODE_NAMES    = 2;
  static uint8_t const LABEL_NODE_NAME     = 3;
  static uint8_t const OUTPUT_NODE_NAME    = 4;
  static uint8_t const LEARNING_RATE       = 5;
  static uint8_t const LEARNING_RATE_PARAM = 6;
  static uint8_t const EPOCH               = 7;

  // private member variables
  static uint8_t const LOSS            = 8;
  static uint8_t const LOSS_SUM        = 9;
  static uint8_t const STEP            = 10;
  static uint8_t const CUMULATIVE_STEP = 11;
  static uint8_t const INPUT_FIRST     = 12;
  static uint8_t const INPUT_SECOND    = 13;
  static uint8_t const CUR_LABEL       = 14;
  static uint8_t const PRED_LABEL      = 15;
  static uint8_t const CUR_TIME        = 16;
  static uint8_t const START_TIME      = 17;
  static uint8_t const TIME_SPAN       = 18;
  static uint8_t const STAT_STRING     = 19;
  static uint8_t const BATCH_DATA      = 20;
  static uint8_t const BATCH_LABELS    = 21;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(21);

    // serialize the graph first
    map.Append(GRAPH, sp.graph_->GetGraphSaveableParams());

    map.Append(INPUT_NODE_NAMES, sp.input_node_names_);
    map.Append(LABEL_NODE_NAME, sp.label_node_name_);
    map.Append(OUTPUT_NODE_NAME, sp.output_node_name_);
    map.Append(LEARNING_RATE, sp.learning_rate_);
    map.Append(LEARNING_RATE_PARAM, sp.learning_rate_param_);

    map.Append(EPOCH, sp.epoch_);
    map.Append(LOSS, sp.loss_);
    map.Append(LOSS_SUM, sp.loss_sum_);
    map.Append(STEP, sp.step_);
    map.Append(CUMULATIVE_STEP, sp.cumulative_step_);

    map.Append(INPUT_FIRST, sp.input_.first);
    map.Append(INPUT_SECOND, sp.input_.second);

    map.Append(CUR_LABEL, sp.cur_label_);
    map.Append(PRED_LABEL, sp.pred_label_);

    map.Append(STAT_STRING, sp.stat_string_);
    map.Append(BATCH_DATA, sp.batch_data_);
    map.Append(BATCH_LABELS, sp.batch_labels_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    // deserialize the graph first
    fetch::ml::GraphSaveableParams<TensorType> gsp;
    map.ExpectKeyGetValue(GRAPH, gsp);
    auto graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
    ml::utilities::BuildGraph(gsp, graph_ptr);
    sp.graph_ = graph_ptr;

    map.ExpectKeyGetValue(INPUT_NODE_NAMES, sp.input_node_names_);
    map.ExpectKeyGetValue(LABEL_NODE_NAME, sp.label_node_name_);
    map.ExpectKeyGetValue(OUTPUT_NODE_NAME, sp.output_node_name_);
    map.ExpectKeyGetValue(LEARNING_RATE, sp.learning_rate_);
    map.ExpectKeyGetValue(LEARNING_RATE_PARAM, sp.learning_rate_param_);

    // recover gradients and gradient trainables from graph
    sp.Init();

    map.ExpectKeyGetValue(EPOCH, sp.epoch_);
    map.ExpectKeyGetValue(LOSS, sp.loss_);
    map.ExpectKeyGetValue(LOSS_SUM, sp.loss_sum_);
    map.ExpectKeyGetValue(STEP, sp.step_);
    map.ExpectKeyGetValue(CUMULATIVE_STEP, sp.cumulative_step_);

    map.ExpectKeyGetValue(INPUT_FIRST, sp.input_.first);
    map.ExpectKeyGetValue(INPUT_SECOND, sp.input_.second);

    map.ExpectKeyGetValue(CUR_LABEL, sp.cur_label_);
    map.ExpectKeyGetValue(PRED_LABEL, sp.pred_label_);

    map.ExpectKeyGetValue(STAT_STRING, sp.stat_string_);
    map.ExpectKeyGetValue(BATCH_DATA, sp.batch_data_);
    map.ExpectKeyGetValue(BATCH_LABELS, sp.batch_labels_);
  }
};

/**
 * serializer for SGDOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::AdamOptimiser<TensorType>, D>
{
  using Type                          = ml::optimisers::AdamOptimiser<TensorType>;
  using DriverType                    = D;
  static uint8_t const BASE_OPTIMISER = 1;
  static uint8_t const CACHE          = 2;
  static uint8_t const MOMENTUM       = 3;
  static uint8_t const MT             = 4;
  static uint8_t const VT             = 5;
  static uint8_t const BETA1          = 6;
  static uint8_t const BETA2          = 7;
  static uint8_t const BETA1_T        = 8;
  static uint8_t const BETA2_T        = 9;
  static uint8_t const EPSILON        = 10;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(10);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> const *>(&sp);
    map.Append(BASE_OPTIMISER, *base_pointer);

    map.Append(CACHE, sp.cache_);
    map.Append(MOMENTUM, sp.momentum_);
    map.Append(MT, sp.mt_);
    map.Append(VT, sp.vt_);
    map.Append(BETA1, sp.beta1_);
    map.Append(BETA2, sp.beta2_);
    map.Append(BETA1_T, sp.beta1_t_);
    map.Append(BETA2_T, sp.beta2_t_);
    map.Append(EPSILON, sp.epsilon_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPTIMISER, *base_pointer);

    map.ExpectKeyGetValue(CACHE, sp.cache_);
    map.ExpectKeyGetValue(MOMENTUM, sp.momentum_);
    map.ExpectKeyGetValue(MT, sp.mt_);
    map.ExpectKeyGetValue(VT, sp.vt_);
    map.ExpectKeyGetValue(BETA1, sp.beta1_);
    map.ExpectKeyGetValue(BETA2, sp.beta2_);
    map.ExpectKeyGetValue(BETA1_T, sp.beta1_t_);
    map.ExpectKeyGetValue(BETA2_T, sp.beta2_t_);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon_);
  }
};

/**
 * serializer for LazyAdamOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::LazyAdamOptimiser<TensorType>, D>
{
  using Type                              = ml::optimisers::LazyAdamOptimiser<TensorType>;
  using DriverType                        = D;
  static uint8_t const BASE_OPTIMISER     = 1;
  static uint8_t const CACHE              = 2;
  static uint8_t const MOMENTUM           = 3;
  static uint8_t const MT                 = 4;
  static uint8_t const VT                 = 5;
  static uint8_t const BETA1              = 6;
  static uint8_t const BETA2              = 7;
  static uint8_t const BETA1_T            = 8;
  static uint8_t const BETA2_T            = 9;
  static uint8_t const SPARSITY_THRESHOLD = 10;
  static uint8_t const EPSILON            = 11;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(11);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> const *>(&sp);
    map.Append(BASE_OPTIMISER, *base_pointer);

    map.Append(CACHE, sp.cache_);
    map.Append(MOMENTUM, sp.momentum_);
    map.Append(MT, sp.mt_);
    map.Append(VT, sp.vt_);
    map.Append(BETA1, sp.beta1_);
    map.Append(BETA2, sp.beta2_);
    map.Append(BETA1_T, sp.beta1_t_);
    map.Append(BETA2_T, sp.beta2_t_);
    map.Append(SPARSITY_THRESHOLD, sp.sparsity_threshold);
    map.Append(EPSILON, sp.epsilon_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::optimisers::Optimiser<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_OPTIMISER, *base_pointer);

    map.ExpectKeyGetValue(CACHE, sp.cache_);
    map.ExpectKeyGetValue(MOMENTUM, sp.momentum_);
    map.ExpectKeyGetValue(MT, sp.mt_);
    map.ExpectKeyGetValue(VT, sp.vt_);
    map.ExpectKeyGetValue(BETA1, sp.beta1_);
    map.ExpectKeyGetValue(BETA2, sp.beta2_);
    map.ExpectKeyGetValue(BETA1_T, sp.beta1_t_);
    map.ExpectKeyGetValue(BETA2_T, sp.beta2_t_);
    map.ExpectKeyGetValue(SPARSITY_THRESHOLD, sp.sparsity_threshold);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon_);
  }
};

/**
 * serializer for Learning Rate Params
 * @tparam TensorType
 */
template <typename T, typename D>
struct MapSerializer<ml::optimisers::LearningRateParam<T>, D>
{
  using Type       = ml::optimisers::LearningRateParam<T>;
  using DriverType = D;

  static uint8_t const LEARNING_RATE_DECAY_MODE = 1;
  static uint8_t const STARTING_LEARNING_RATE   = 2;
  static uint8_t const ENDING_LEARNING_RATE     = 3;
  static uint8_t const LINEAR_DECAY_RATE        = 4;
  static uint8_t const EXPONENTIAL_DECAY_RATE   = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);

    map.Append(LEARNING_RATE_DECAY_MODE, static_cast<uint8_t>(sp.mode));

    map.Append(STARTING_LEARNING_RATE, sp.starting_learning_rate);
    map.Append(ENDING_LEARNING_RATE, sp.ending_learning_rate);
    map.Append(LINEAR_DECAY_RATE, sp.linear_decay_rate);
    map.Append(EXPONENTIAL_DECAY_RATE, sp.exponential_decay_rate);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    uint8_t lrdm;
    map.ExpectKeyGetValue(LEARNING_RATE_DECAY_MODE, lrdm);
    sp.mode =
        static_cast<typename fetch::ml::optimisers::LearningRateParam<T>::LearningRateDecay>(lrdm);

    map.ExpectKeyGetValue(STARTING_LEARNING_RATE, sp.starting_learning_rate);
    map.ExpectKeyGetValue(ENDING_LEARNING_RATE, sp.ending_learning_rate);
    map.ExpectKeyGetValue(LINEAR_DECAY_RATE, sp.linear_decay_rate);
    map.ExpectKeyGetValue(EXPONENTIAL_DECAY_RATE, sp.exponential_decay_rate);
  }
};

/**
 * serializer for MinMaxScaler
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::utilities::MinMaxScaler<TensorType>, D>
{
  using Type       = ml::utilities::MinMaxScaler<TensorType>;
  using DriverType = D;

  static uint8_t const MIN_VAL = 1;
  static uint8_t const MAX_VAL = 2;
  static uint8_t const RANGE   = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    map.Append(MIN_VAL, sp.x_min_);
    map.Append(MAX_VAL, sp.x_max_);
    map.Append(RANGE, sp.x_range_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(MIN_VAL, sp.x_min_);
    map.ExpectKeyGetValue(MAX_VAL, sp.x_max_);
    map.ExpectKeyGetValue(RANGE, sp.x_range_);
  }
};

}  // namespace serializers
}  // namespace fetch
