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
#include "core/serializers/map_serializer_boilerplate.hpp"
#include "meta/value_util.hpp"
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
  : MapSerializerBoilerplate<ml::OpsSaveableParams, D,
                             EXPECTED_KEY_MEMBER(1, ml::OpsSaveableParams::op_type),
                             EXPECTED_KEY_MEMBER(2, ml::OpsSaveableParams::is_training),
                             EXPECTED_KEY_MEMBER(3, ml::OpsSaveableParams::batch_input_shapes),
                             EXPECTED_KEY_MEMBER(4, ml::OpsSaveableParams::batch_output_shape)>
{
};

/**
 * serializer for OpType
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<ml::OpType, D>
  : MapSerializerBoilerplate<ml::OpType, D, SimplySerializedAs<1, uint16_t>>
{
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
  : MapSerializerBoilerplate<
        ml::SubGraphSaveableParams<TensorType>, D,
        // serialize parent class first
        SimplySerializedAs<1, ml::GraphSaveableParams<TensorType>>,
        // serialize parent class first
        SimplySerializedAs<2, ml::OpsSaveableParams>,

        EXPECTED_KEY_MEMBER(3, ml::SubGraphSaveableParams<TensorType>::op_type),
        EXPECTED_KEY_MEMBER(4, ml::SubGraphSaveableParams<TensorType>::input_node_names),
        EXPECTED_KEY_MEMBER(5, ml::SubGraphSaveableParams<TensorType>::output_node_name)>
{
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
 * Few of the serializers below follow this pattern.
 */
template <class T, typename D, class Base = ml::OpsSaveableParams, class... Fields>
struct BaseAndOpTypeSerializer
{
  using Type       = T;
  using DriverType = D;

  static uint8_t const BASE    = 1;
  static uint8_t const OP_CODE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &v)
  {
    auto map = map_constructor(2 + sizeof...(Fields));

    // serialize parent class first
    map.Append(BASE, static_cast<Base const &>(v));

    map.Append(OP_CODE, v.op_type);
    value_util::ForEach([&map, &v](auto field) { field.Serialize(map, v); }, Fields{}...);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &v)
  {
    map.ExpectKeyGetValue(BASE, static_cast<Base &>(v));

    map.ExpectKeyGetValue(OP_CODE, v.op_type);
    value_util::ForEach([&map, &v](auto field) { field.Deserialize(map, v); }, Fields{}...);
  }
};

/**
 * serializer for abs saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAbsSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpAbsSaveableParams<TensorType>, D>
{
};

/**
 * serializer for add saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAddSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpAddSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3, ml::OpAddSaveableParams<TensorType>::axes)>
{
};

/**
 * serializer for concatenate saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConcatenateSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpConcatenateSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3,
                                                ml::OpConcatenateSaveableParams<TensorType>::axis)>
{
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

  static uint8_t const BASE_OPS     = 1;
  static uint8_t const OP_CODE      = 2;
  static uint8_t const DATA         = 3;
  static uint8_t const DATA_PRESENT = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto ops_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> const *>(&sp);
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
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto ops_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> *>(&sp);
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
  }
};

/**
 * serializer for conv1d saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConvolution1DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpConvolution1DSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(
                                3, ml::OpConvolution1DSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for conv2d saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpConvolution2DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpConvolution2DSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(
                                3, ml::OpConvolution2DSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for OpCrossEntropyLossSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpCrossEntropyLossSaveableParams<TensorType>, D>
  : MapSerializerBoilerplate<ml::OpCrossEntropyLossSaveableParams<TensorType>, D,
                             EXPECTED_KEY_MEMBER(
                                 1, ml::OpCrossEntropyLossSaveableParams<TensorType>::op_type),
                             SimplySerializedAs<2, ml::OpsSaveableParams>>
{
};

/**
 * serializer for OpDataHolderSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpDataHolderSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpDataHolderSaveableParams<TensorType>, D>
{
};

/**
 * serializer for divide saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpDivideSaveableParams<TensorType>, D>
  : MapSerializerBoilerplate<ml::OpDivideSaveableParams<TensorType>, D,
                             EXPECTED_KEY_MEMBER(1,
                                                 ml::OpDivideSaveableParams<TensorType>::op_type),
                             SimplySerializedAs<2, ml::OpsSaveableParams>>
{
};

/**
 * serializer for divide saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpDropoutSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpDropoutSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpDropoutSaveableParams<TensorType>::random_seed),
        EXPECTED_KEY_MEMBER(4, ml::OpDropoutSaveableParams<TensorType>::probability),
        EXPECTED_KEY_MEMBER(5, ml::OpDropoutSaveableParams<TensorType>::buffer),
        EXPECTED_KEY_MEMBER(6, ml::OpDropoutSaveableParams<TensorType>::index)>
{
};

/**
 * serializer for Elu saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpEluSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpEluSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3, ml::OpEluSaveableParams<TensorType>::a)>
{
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpEmbeddingsSaveableParams<TensorType>, D>
  : MapSerializerBoilerplate<ml::OpEmbeddingsSaveableParams<TensorType>, D,
                             EXPECTED_KEY_MEMBER(
                                 1, ml::OpEmbeddingsSaveableParams<TensorType>::op_type),
                             SimplySerializedAs<2, ml::OpWeightsSaveableParams<TensorType>>>
{
};

/**
 * serializer for Exp saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpExpSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpExpSaveableParams<TensorType>, D>
{
};

/**
 * serializer for Flatten saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpFlattenSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpFlattenSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(
                                3, ml::OpFlattenSaveableParams<TensorType>::input_shape)>
{
};

/**
 * serializer for Elu saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpGeluSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpGeluSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpLayerNormSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLayerNormSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpLayerNormSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3,
                                                ml::OpLayerNormSaveableParams<TensorType>::epsilon),
                            EXPECTED_KEY_MEMBER(4, ml::OpLayerNormSaveableParams<TensorType>::axis)>
{
};

/**
 * serializer for LeakyRelu saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLeakyReluSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpLeakyReluSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3, ml::OpLeakyReluSaveableParams<TensorType>::a)>
{
};

/**
 * serializer for PReluOp saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpPReluOpSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpPReluOpSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3, ml::OpPReluOpSaveableParams<TensorType>::a)>
{
};

/**
 * serializer for Log saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpLogSaveableParams<TensorType>, D>
{
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSigmoidSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpLogSigmoidSaveableParams<TensorType>, D>
{
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSoftmaxSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpLogSoftmaxSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3,
                                                ml::OpLogSoftmaxSaveableParams<TensorType>::axis)>
{
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaskFillSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpMaskFillSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(
                                3, ml::OpMaskFillSaveableParams<TensorType>::fill_value)>
{
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMatrixMultiplySaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpMatrixMultiplySaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpMatrixMultiplySaveableParams<TensorType>::transpose_a),
        EXPECTED_KEY_MEMBER(4, ml::OpMatrixMultiplySaveableParams<TensorType>::transpose_b)>
{
};

/**
 * serializer for MaxPool1D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPool1DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpMaxPool1DSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpMaxPool1DSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::OpMaxPool1DSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for MaxPool saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPoolSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpMaxPoolSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpMaxPoolSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::OpMaxPoolSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for MaxPool2D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPool2DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpMaxPool2DSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpMaxPool2DSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::OpMaxPool2DSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for AvgPool1D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAvgPool1DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpAvgPool1DSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpAvgPool1DSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::OpAvgPool1DSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for Average Pool 2D saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpAvgPool2DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpAvgPool2DSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpAvgPool2DSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::OpAvgPool2DSaveableParams<TensorType>::stride_size)>
{
};

/**
 * serializer for Mean Square Error saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMeanSquareErrorSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpMeanSquareErrorSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpMeanSquareErrorSaveableParams<TensorType>::weightings)>
{
};

/**
 * serializer for CategoricalAccuracy saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpCategoricalAccuracySaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpCategoricalAccuracySaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpCategoricalAccuracySaveableParams<TensorType>::weightings)>
{
};

/**
 * serializer for OpMaximumSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaximumSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpMaximumSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpMultiplySaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMultiplySaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpMultiplySaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpPlaceholderSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpPlaceholderSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpPlaceholderSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpRandomisedReluSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpRandomisedReluSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpRandomisedReluSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpRandomisedReluSaveableParams<TensorType>::lower_bound),
        EXPECTED_KEY_MEMBER(4, ml::OpRandomisedReluSaveableParams<TensorType>::upper_bound),
        EXPECTED_KEY_MEMBER(5, ml::OpRandomisedReluSaveableParams<TensorType>::random_seed),
        EXPECTED_KEY_MEMBER(6, ml::OpRandomisedReluSaveableParams<TensorType>::buffer),
        EXPECTED_KEY_MEMBER(7, ml::OpRandomisedReluSaveableParams<TensorType>::index),
        EXPECTED_KEY_MEMBER(8, ml::OpRandomisedReluSaveableParams<TensorType>::random_value)>
{
};

/**
 * serializer for OpReluSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpReluSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpReluSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpReshapeSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpReshapeSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpReshapeSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpReshapeSaveableParams<TensorType>::new_shape),
        EXPECTED_KEY_MEMBER(4, ml::OpReshapeSaveableParams<TensorType>::new_size)>
{
};

/**
 * serializer for OpSigmoidSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSigmoidSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSigmoidSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpSliceaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSliceSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpSliceSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpSliceSaveableParams<TensorType>::axes),
        EXPECTED_KEY_MEMBER(4, ml::OpSliceSaveableParams<TensorType>::indices),
        EXPECTED_KEY_MEMBER(5, ml::OpSliceSaveableParams<TensorType>::axis),
        EXPECTED_KEY_MEMBER(6, ml::OpSliceSaveableParams<TensorType>::index),
        EXPECTED_KEY_MEMBER(7, ml::OpSliceSaveableParams<TensorType>::slice_type),
        EXPECTED_KEY_MEMBER(8, ml::OpSliceSaveableParams<TensorType>::start_end_slice)>
{
};

/**
 * serializer for OpStridedSliceaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpStridedSliceSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpStridedSliceSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpStridedSliceSaveableParams<TensorType>::begins),
        EXPECTED_KEY_MEMBER(4, ml::OpStridedSliceSaveableParams<TensorType>::ends),
        EXPECTED_KEY_MEMBER(5, ml::OpStridedSliceSaveableParams<TensorType>::strides)>
{
};

/**
 * serializer for OpSqueezeaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSqueezeSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSqueezeSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpReduceMeanaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpReduceMeanSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpReduceMeanSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3,
                                                ml::OpReduceMeanSaveableParams<TensorType>::axis)>
{
};

/**
 * serializer for OpSoftmaxSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSoftmaxSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSoftmaxSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3, ml::OpSoftmaxSaveableParams<TensorType>::axis),
                            EXPECTED_KEY_MEMBER(4, ml::OpSoftmaxSaveableParams<TensorType>::axes)>
{
};

/**
 * serializer for OpSoftmaxCrossEntropySaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSoftmaxCrossEntropySaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSoftmaxCrossEntropySaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpSwitchSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSwitchSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSwitchSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpSQRTSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSQRTSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSQRTSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpSubtractSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpSubtractSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpSubtractSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpTanhSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpTanhSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpTanhSaveableParams<TensorType>, D>
{
};

/**
 * serializer for OpTransposeSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpTransposeSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpTransposeSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(
                                3, ml::OpTransposeSaveableParams<TensorType>::transpose_vector)>
{
};

/**
 * serializer for OpOneHotSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpOneHotSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::OpOneHotSaveableParams<TensorType>, D, ml::OpsSaveableParams,
        EXPECTED_KEY_MEMBER(3, ml::OpOneHotSaveableParams<TensorType>::depth),
        EXPECTED_KEY_MEMBER(4, ml::OpOneHotSaveableParams<TensorType>::axis),
        EXPECTED_KEY_MEMBER(5, ml::OpOneHotSaveableParams<TensorType>::on_value),
        EXPECTED_KEY_MEMBER(6, ml::OpOneHotSaveableParams<TensorType>::off_value)>
{
};

/**
 * serializer for OpTopKSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpTopKSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::OpTopKSaveableParams<TensorType>, D, ml::OpsSaveableParams,
                            EXPECTED_KEY_MEMBER(3, ml::OpTopKSaveableParams<TensorType>::k),
                            EXPECTED_KEY_MEMBER(4, ml::OpTopKSaveableParams<TensorType>::sorted)>
{
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

  static uint8_t const DATA         = 3;
  static uint8_t const DATA_PRESENT = 4;

  static uint8_t const REGULARISATION_TYPE = 5;
  static uint8_t const REGULARISATION_RATE = 6;

  static uint8_t const HAS_GRADIENT          = 7;
  static uint8_t const GRADIENT_ACCUMULATION = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(8);

    // serialize parent class first
    auto base_pointer = static_cast<ml::OpDataHolderSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);

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

    bool has_data = true;
    map.ExpectKeyGetValue(DATA_PRESENT, has_data);
    if (has_data)
    {
      TensorType data;
      map.ExpectKeyGetValue(DATA, data);
      sp.data = std::make_shared<TensorType>(data);
    }

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
  : BaseAndOpTypeSerializer<ml::OpWeightsSaveableParams<TensorType>, D>
{
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
  : BaseAndOpTypeSerializer<
        ml::LayerConvolution1DSaveableParams<TensorType>, D, ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerConvolution1DSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::LayerConvolution1DSaveableParams<TensorType>::input_channels),
        EXPECTED_KEY_MEMBER(5, ml::LayerConvolution1DSaveableParams<TensorType>::output_channels),
        EXPECTED_KEY_MEMBER(6, ml::LayerConvolution1DSaveableParams<TensorType>::stride_size),
        EXPECTED_KEY_MEMBER(7, ml::LayerConvolution1DSaveableParams<TensorType>::is_initialized)>
{
};

/**
 * serializer for Conv2d layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerConvolution2DSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerConvolution2DSaveableParams<TensorType>, D, ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerConvolution2DSaveableParams<TensorType>::kernel_size),
        EXPECTED_KEY_MEMBER(4, ml::LayerConvolution2DSaveableParams<TensorType>::input_channels),
        EXPECTED_KEY_MEMBER(5, ml::LayerConvolution2DSaveableParams<TensorType>::output_channels),
        EXPECTED_KEY_MEMBER(6, ml::LayerConvolution2DSaveableParams<TensorType>::stride_size)>
{
<<<<<<< HEAD
=======
  using Type       = ml::LayerConvolution2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH       = 1;
  static uint8_t const OP_CODE         = 2;
  static uint8_t const KERNEL_SIZE     = 3;
  static uint8_t const INPUT_CHANNELS  = 4;
  static uint8_t const OUTPUT_CHANNELS = 5;
  static uint8_t const STRIDE_SIZE     = 6;
  static uint8_t const IS_INITIALISED  = 7;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(IS_INITIALISED);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OUTPUT_CHANNELS, sp.output_channels);
    map.Append(STRIDE_SIZE, sp.stride_size);
    map.Append(IS_INITIALISED, sp.is_initialised);
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
  }
>>>>>>> master
};

/**
 * serializer for FullyConnectedLayer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerFullyConnectedSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerFullyConnectedSaveableParams<TensorType>, D,
        ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerFullyConnectedSaveableParams<TensorType>::total_inputs_),
        EXPECTED_KEY_MEMBER(4, ml::LayerFullyConnectedSaveableParams<TensorType>::total_outputs_),
        EXPECTED_KEY_MEMBER(5, ml::LayerFullyConnectedSaveableParams<TensorType>::time_distributed),
        EXPECTED_KEY_MEMBER(6, ml::LayerFullyConnectedSaveableParams<TensorType>::is_initialised),
        EXPECTED_KEY_MEMBER(7, ml::LayerFullyConnectedSaveableParams<TensorType>::weights_name),
        EXPECTED_KEY_MEMBER(8, ml::LayerFullyConnectedSaveableParams<TensorType>::bias_name),
        EXPECTED_KEY_MEMBER(9, ml::LayerFullyConnectedSaveableParams<TensorType>::init_mode)>
{
};

/**
 * serializer for OpMaximumSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerLayerNormSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerLayerNormSaveableParams<TensorType>, D, ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerLayerNormSaveableParams<TensorType>::data_shape),
        EXPECTED_KEY_MEMBER(4, ml::LayerLayerNormSaveableParams<TensorType>::axis),
        EXPECTED_KEY_MEMBER(5, ml::LayerLayerNormSaveableParams<TensorType>::epsilon)>
{
};

/**
 * serializer for LayerMultiHeadSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerMultiHeadSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerMultiHeadSaveableParams<TensorType>, D, ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerMultiHeadSaveableParams<TensorType>::value_dim),
        EXPECTED_KEY_MEMBER(4, ml::LayerMultiHeadSaveableParams<TensorType>::key_dim),
        EXPECTED_KEY_MEMBER(5, ml::LayerMultiHeadSaveableParams<TensorType>::n_heads),
        EXPECTED_KEY_MEMBER(6, ml::LayerMultiHeadSaveableParams<TensorType>::model_dim),
        EXPECTED_KEY_MEMBER(7, ml::LayerMultiHeadSaveableParams<TensorType>::dropout)>
{
};

/**
 * serializer for PRELU layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerPReluSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<ml::LayerPReluSaveableParams<TensorType>, D,
                            ml::SubGraphSaveableParams<TensorType>>
{
};

/**
 * serializer for LayerScaledDotProductAttentionSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerScaledDotProductAttentionSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerScaledDotProductAttentionSaveableParams<TensorType>, D,
        ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3,
                            ml::LayerScaledDotProductAttentionSaveableParams<TensorType>::key_dim),
        EXPECTED_KEY_MEMBER(4,
                            ml::LayerScaledDotProductAttentionSaveableParams<TensorType>::dropout)>
{
};

/**
 * serializer for self attention layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerSelfAttentionEncoderSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerSelfAttentionEncoderSaveableParams<TensorType>, D,
        ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>::n_heads),
        EXPECTED_KEY_MEMBER(4, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>::model_dim),
        EXPECTED_KEY_MEMBER(5, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>::ff_dim),
        EXPECTED_KEY_MEMBER(
            6, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>::residual_dropout),
        EXPECTED_KEY_MEMBER(
            7, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>::attention_dropout),
        EXPECTED_KEY_MEMBER(
            8, ml::LayerSelfAttentionEncoderSaveableParams<TensorType>::feedforward_dropout)>
{
};

/**
 * serializer for LayerSkipGramSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LayerSkipGramSaveableParams<TensorType>, D>
  : BaseAndOpTypeSerializer<
        ml::LayerSkipGramSaveableParams<TensorType>, D, ml::SubGraphSaveableParams<TensorType>,
        EXPECTED_KEY_MEMBER(3, ml::LayerSkipGramSaveableParams<TensorType>::in_size),
        EXPECTED_KEY_MEMBER(4, ml::LayerSkipGramSaveableParams<TensorType>::out_size),
        EXPECTED_KEY_MEMBER(5, ml::LayerSkipGramSaveableParams<TensorType>::embed_in)>
{
};

/**
 * serializer for Optimiser
 * @tparam TensorType
 */
template <uint8_t KEY, class TensorType>
struct OptimiserGetGraphSaveableParams : ValueSerializer
{
  template <class Map, class Object>
  static constexpr void Serialize(Map &map, Object const &object)
  {
    map.Append(KEY, object.graph_->GetGraphSaveableParams());
  }
  template <class Map, class Object>
  static constexpr void Deserializer(Map &map, Object &object)
  {
    fetch::ml::GraphSaveableParams<TensorType> gsp;
    map.ExpectKeyGetValue(KEY, gsp);
    auto graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
    ml::utilities::BuildGraph(gsp, graph_ptr);
    object.graph_ = graph_ptr;
  }
};

template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::Optimiser<TensorType>, D>
  : MapSerializerBoilerplate<
        ml::optimisers::Optimiser<TensorType>, D, OptimiserGetGraphSaveableParams<1, TensorType>,
        EXPECTED_KEY_MEMBER(2, ml::optimisers::Optimiser<TensorType>::input_node_names_),
        EXPECTED_KEY_MEMBER(3, ml::optimisers::Optimiser<TensorType>::label_node_name_),
        EXPECTED_KEY_MEMBER(4, ml::optimisers::Optimiser<TensorType>::output_node_name_),
        EXPECTED_KEY_MEMBER(5, ml::optimisers::Optimiser<TensorType>::learning_rate_),
        EXPECTED_KEY_MEMBER(6, ml::optimisers::Optimiser<TensorType>::learning_rate_param_),
        EXPECTED_KEY_MEMBER(7, ml::optimisers::Optimiser<TensorType>::epoch_),
        EXPECTED_KEY_MEMBER(8, ml::optimisers::Optimiser<TensorType>::loss_),
        EXPECTED_KEY_MEMBER(9, ml::optimisers::Optimiser<TensorType>::loss_sum_),
        EXPECTED_KEY_MEMBER(10, ml::optimisers::Optimiser<TensorType>::step_),
        EXPECTED_KEY_MEMBER(11, ml::optimisers::Optimiser<TensorType>::cumulative_step_),
        EXPECTED_KEY_MEMBER(12, ml::optimisers::Optimiser<TensorType>::first),
        EXPECTED_KEY_MEMBER(13, ml::optimisers::Optimiser<TensorType>::second),
        EXPECTED_KEY_MEMBER(14, ml::optimisers::Optimiser<TensorType>::cur_label_),
        EXPECTED_KEY_MEMBER(15, ml::optimisers::Optimiser<TensorType>::pred_label_),
        EXPECTED_KEY_MEMBER(16, ml::optimisers::Optimiser<TensorType>::stat_string_),
        EXPECTED_KEY_MEMBER(17, ml::optimisers::Optimiser<TensorType>::batch_data_),
        EXPECTED_KEY_MEMBER(18, ml::optimisers::Optimiser<TensorType>::batch_labels_)>
{
};

/**
 * serializer for SGDOptimiser
 * @tparam TensorType
 */
template <class TensorType>
struct AdamOptimiserSerialiser
{
  template <typename D>
  struct Impl : MapSerializerBoilerplate<
                    ml::optimisers::AdamOptimiser<TensorType>, D,
                    SimplySerializedAs<1, ml::optimisers::Optimiser<TensorType>>,
                    EXPECTED_KEY_MEMBER(2, ml::optimisers::AdamOptimiser<TensorType>::cache_),
                    EXPECTED_KEY_MEMBER(3, ml::optimisers::AdamOptimiser<TensorType>::momentum_),
                    EXPECTED_KEY_MEMBER(4, ml::optimisers::AdamOptimiser<TensorType>::mt_),
                    EXPECTED_KEY_MEMBER(5, ml::optimisers::AdamOptimiser<TensorType>::vt_),
                    EXPECTED_KEY_MEMBER(6, ml::optimisers::AdamOptimiser<TensorType>::beta1_),
                    EXPECTED_KEY_MEMBER(7, ml::optimisers::AdamOptimiser<TensorType>::beta2_),
                    EXPECTED_KEY_MEMBER(8, ml::optimisers::AdamOptimiser<TensorType>::beta1_t_),
                    EXPECTED_KEY_MEMBER(9, ml::optimisers::AdamOptimiser<TensorType>::beta2_t_),
                    EXPECTED_KEY_MEMBER(10, ml::optimisers::AdamOptimiser<TensorType>::epsilon_)>
  {
  };
};

template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::AdamOptimiser<TensorType>, D>
  : AdamOptimiserSerialiser<TensorType>::template Impl<D>
{
};

/**
 * serializer for LazyAdamOptimiser
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::optimisers::LazyAdamOptimiser<TensorType>, D>
  : MapSerializerBoilerplate<
        ml::optimisers::LazyAdamOptimiser<TensorType>, D,
        SimplySerializedAs<1, ml::optimisers::Optimiser<TensorType>>,
        EXPECTED_KEY_MEMBER(2, ml::optimisers::LazyAdamOptimiser<TensorType>::cache_),
        EXPECTED_KEY_MEMBER(3, ml::optimisers::LazyAdamOptimiser<TensorType>::momentum_),
        EXPECTED_KEY_MEMBER(4, ml::optimisers::LazyAdamOptimiser<TensorType>::mt_),
        EXPECTED_KEY_MEMBER(5, ml::optimisers::LazyAdamOptimiser<TensorType>::vt_),
        EXPECTED_KEY_MEMBER(6, ml::optimisers::LazyAdamOptimiser<TensorType>::beta1_),
        EXPECTED_KEY_MEMBER(7, ml::optimisers::LazyAdamOptimiser<TensorType>::beta2_),
        EXPECTED_KEY_MEMBER(8, ml::optimisers::LazyAdamOptimiser<TensorType>::beta1_t_),
        EXPECTED_KEY_MEMBER(9, ml::optimisers::LazyAdamOptimiser<TensorType>::beta2_t_),
        EXPECTED_KEY_MEMBER(10, ml::optimisers::LazyAdamOptimiser<TensorType>::sparsity_threshold),
        EXPECTED_KEY_MEMBER(11, ml::optimisers::LazyAdamOptimiser<TensorType>::epsilon_)>
{
};

/**
 * serializer for Learning Rate Params
 * @tparam TensorType
 */
template <typename T, typename D>
struct MapSerializer<ml::optimisers::LearningRateParam<T>, D>
  : MapSerializerBoilerplate<
        ml::optimisers::LearningRateParam<T>, D,
        EXPECTED_KEY_MEMBER_AS(1, ml::optimisers::LearningRateParam<T>::mode, uint8_t),
        EXPECTED_KEY_MEMBER(2, ml::optimisers::LearningRateParam<T>::starting_learning_rate),
        EXPECTED_KEY_MEMBER(3, ml::optimisers::LearningRateParam<T>::ending_learning_rate),
        EXPECTED_KEY_MEMBER(4, ml::optimisers::LearningRateParam<T>::linear_decay_rate),
        EXPECTED_KEY_MEMBER(5, ml::optimisers::LearningRateParam<T>::exponential_decay_rate)>
{
};

/**
 * serializer for MinMaxScaler
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::utilities::MinMaxScaler<TensorType>, D>
  : MapSerializerBoilerplate<
        ml::utilities::MinMaxScaler<TensorType>, D,
        EXPECTED_KEY_MEMBER(1, ml::utilities::MinMaxScaler<TensorType>::x_min_),
        EXPECTED_KEY_MEMBER(2, ml::utilities::MinMaxScaler<TensorType>::x_max_),
        EXPECTED_KEY_MEMBER(3, ml::utilities::MinMaxScaler<TensorType>::x_range_)>
{
};

}  // namespace serializers
}  // namespace fetch
