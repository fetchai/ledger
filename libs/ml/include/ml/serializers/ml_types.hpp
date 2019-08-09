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

#include "ml/saveparams/saveable_params.hpp"

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"

#include "ml/regularisers/reg_types.hpp"

namespace fetch {
namespace ml {

template <typename T>
struct StateDict;

}  // namespace ml

namespace serializers {

///////////////////////////////////////////
/// OP SPECIFIC SERIALISATION FUNCTIONS ///
///////////////////////////////////////////

namespace {
template <class TensorType, typename D, class SP, typename MapType>
void SerializeImplementation(MapType &map, uint8_t code,
                             std::shared_ptr<fetch::ml::SaveableParamsInterface> op)
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
}  // namespace

template <class TensorType, typename D, typename MapType>
void SerializeAnyOp(MapType &map, uint8_t code, fetch::ml::OpType const &op_type,
                    std::shared_ptr<fetch::ml::SaveableParamsInterface> op)
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
  case ml::OpType::OP_CROSS_ENTROPY_LOSS:
  {
    SerializeImplementation<TensorType, D, ml::OpCrossEntropyLossSaveableParams<TensorType>>(
        map, code, op);
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
  case ml::OpType::OP_LEAKY_RELU_OP:
  {
    SerializeImplementation<TensorType, D, ml::OpLeakyReluOpSaveableParams<TensorType>>(map, code,
                                                                                        op);
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
  case ml::OpType::OP_MEAN_SQUARE_ERROR_LOSS:
  {
    SerializeImplementation<TensorType, D, ml::OpMeanSquareErrorSaveableParams<TensorType>>(
        map, code, op);
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
  case ml::OpType::OP_SOFTMAX_CROSS_ENTROPY_LOSS:
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
  default:
  {
    throw std::runtime_error("Unknown type for Serialization");
  }
  }
}

template <class TensorType, typename D, typename MapType>
void DeserializeAnyOp(MapType &map, uint8_t code, fetch::ml::OpType const &op_type,
                      std::shared_ptr<fetch::ml::SaveableParamsInterface> &op)
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
  case ml::OpType::OP_CROSS_ENTROPY_LOSS:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpCrossEntropyLossSaveableParams<TensorType>>(
        map, code);
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
  case ml::OpType::OP_LEAKY_RELU_OP:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpLeakyReluOpSaveableParams<TensorType>>(
        map, code);
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
  case ml::OpType::OP_MATRIX_MULTIPLY:
  {
    op = DeserializeImplementation<TensorType, D, ml::OpMatrixMultiplySaveableParams<TensorType>>(
        map, code);
    break;
  }
  case ml::OpType::OP_MEAN_SQUARE_ERROR_LOSS:
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
  case ml::OpType::OP_SOFTMAX_CROSS_ENTROPY_LOSS:
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
  default:
  {
    throw std::runtime_error("Unknown type for Deserialization");
  }
  }
}

template <typename V, typename D>
struct MapSerializer<ml::StateDict<V>, D>
{
public:
  using Type       = ml::StateDict<V>;
  using DriverType = D;

  constexpr static uint8_t WEIGHTS = 1;
  constexpr static uint8_t DICT    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sd)
  {
    uint64_t n = 0;
    if (sd.weights_)
    {
      ++n;
    }
    if (!sd.dict_.empty())
    {
      ++n;
    }
    auto map = map_constructor(n);
    if (sd.weights_)
    {
      map.Append(WEIGHTS, *sd.weights_);
    }
    if (!sd.dict_.empty())
    {
      map.Append(DICT, sd.dict_);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &output)
  {
    for (uint64_t i = 0; i < map.size(); ++i)
    {
      uint8_t key;
      map.GetKey(key);
      switch (key)
      {
      case WEIGHTS:
        output.weights_ = std::make_shared<V>();
        map.GetValue(*output.weights_);
        break;
      case DICT:
        map.GetValue(output.dict_);
        break;
      default:
        throw std::runtime_error("unsupported key in statemap deserialization");
      }
    }
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
  //  static uint8_t const TRAINABLE_LOOKUP   = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);
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
    for (auto node_name : connections_first)
    {
      auto nsp =
          std::dynamic_pointer_cast<ml::NodeSaveableParams<TensorType>>(sp.nodes.at(node_name));
      nodevec.emplace_back(*nsp);
    }

    map.Append(NODES, nodevec);
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
  static uint8_t const OP_CODE          = 2;
  static uint8_t const INPUT_NODE_NAMES = 3;
  static uint8_t const OUTPUT_NODE_NAME = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto base_pointer = static_cast<ml::GraphSaveableParams<TensorType> const *>(&sp);
    map.Append(GRAPH, *(base_pointer));

    map.Append(OP_CODE, sp.op_type);
    map.Append(INPUT_NODE_NAMES, sp.input_node_names);
    map.Append(OUTPUT_NODE_NAME, sp.output_node_name);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::GraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(GRAPH, (*base_pointer));

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

  static uint8_t const NAME                 = 1;
  static uint8_t const CACHED_OUTPUT        = 2;
  static uint8_t const CACHED_OUTPUT_STATUS = 3;
  static uint8_t const OP_CODE              = 4;
  static uint8_t const OP                   = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);

    map.Append(NAME, sp.name);
    map.Append(CACHED_OUTPUT, sp.cached_output);
    map.Append(CACHED_OUTPUT_STATUS, sp.cached_output_status);
    map.Append(OP_CODE, sp.operation_type);

    SerializeAnyOp<TensorType, D>(map, OP, sp.operation_type, sp.op_save_params);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(NAME, sp.name);
    map.ExpectKeyGetValue(CACHED_OUTPUT, sp.cached_output);
    map.ExpectKeyGetValue(CACHED_OUTPUT_STATUS, sp.cached_output_status);
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;
  static uint8_t const AXES    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXES, sp.axes);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;
  static uint8_t const AXIS    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXIS, sp.axis);
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

  static uint8_t const OP_CODE     = 1;
  static uint8_t const STRIDE_SIZE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE     = 1;
  static uint8_t const STRIDE_SIZE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
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

  static uint8_t const OP_CODE     = 1;
  static uint8_t const RANDOM_SEED = 2;
  static uint8_t const PROBABILITY = 3;
  static uint8_t const BUFFER      = 4;
  static uint8_t const INDEX       = 5;
  static uint8_t const DROP_VALUES = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);
    map.Append(OP_CODE, sp.op_type);
    map.Append(RANDOM_SEED, sp.random_seed);
    map.Append(PROBABILITY, sp.probability);
    map.Append(BUFFER, sp.buffer);
    map.Append(INDEX, sp.index);
    map.Append(DROP_VALUES, sp.drop_values);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(RANDOM_SEED, sp.random_seed);
    map.ExpectKeyGetValue(PROBABILITY, sp.probability);
    map.ExpectKeyGetValue(BUFFER, sp.buffer);
    map.ExpectKeyGetValue(INDEX, sp.index);
    map.ExpectKeyGetValue(DROP_VALUES, sp.drop_values);
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

  static uint8_t const OP_CODE = 1;
  static uint8_t const VALUE   = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(VALUE, sp.a);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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
  static uint8_t const EMBED_OUTPUT = 3;
  static uint8_t const UPDATED_ROWS = 4;
  static uint8_t const TRAILING_IND_1 = 5;
  static uint8_t const TRAILING_IND_2 = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);
    map.Append(OP_CODE, sp.op_type);

    // serialize parent class
    auto base_pointer = static_cast<ml::OpWeightsSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);

    map.Append(EMBED_OUTPUT, *sp.embeddings_output);
    map.Append(UPDATED_ROWS, sp.updated_rows);
    map.Append(TRAILING_IND_1, sp.trailing_indices1);
    map.Append(TRAILING_IND_2, sp.trailing_indices2);

  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    auto base_pointer = static_cast<ml::OpWeightsSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_CLASS, *base_pointer);

    TensorType e_out;
    map.ExpectKeyGetValue(EMBED_OUTPUT, e_out);
    sp.embeddings_output = std::make_shared<TensorType>(e_out);
    map.ExpectKeyGetValue(UPDATED_ROWS, sp.updated_rows);
    map.ExpectKeyGetValue(TRAILING_IND_1, sp.trailing_indices1);
    map.ExpectKeyGetValue(TRAILING_IND_2, sp.trailing_indices2);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpExpSaveableParams<TensorType>, D>
{
  using Type       = ml::OpExpSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpFlattenSaveableParams<TensorType>, D>
{
  using Type       = ml::OpFlattenSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const INPUT_SHAPE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(INPUT_SHAPE, sp.input_shape);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
  }
};

/**
 * serializer for OpMaximumSaveableParams saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLayerNormSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLayerNormSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE             = 1;
  static uint8_t const EPSILON             = 2;
  static uint8_t const AXIS                = 3;
  static uint8_t const PREV_INPUT          = 4;
  static uint8_t const CACHED_INV_SQRT_VAR = 5;
  static uint8_t const CACHED_OUTPUT       = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);
    map.Append(OP_CODE, sp.op_type);
    map.Append(EPSILON, sp.epsilon);
    map.Append(AXIS, sp.axis);
    map.Append(PREV_INPUT, sp.prev_input);
    map.Append(CACHED_INV_SQRT_VAR, sp.cached_inv_sqrt_var);
    map.Append(CACHED_OUTPUT, sp.cached_output);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(EPSILON, sp.epsilon);
    map.ExpectKeyGetValue(AXIS, sp.axis);
    map.ExpectKeyGetValue(PREV_INPUT, sp.prev_input);
    map.ExpectKeyGetValue(CACHED_INV_SQRT_VAR, sp.cached_inv_sqrt_var);
    map.ExpectKeyGetValue(CACHED_OUTPUT, sp.cached_output);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLeakyReluSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLeakyReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;
  static uint8_t const VAL     = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(VAL, sp.a);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(VAL, sp.a);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLeakyReluOpSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLeakyReluOpSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;
  static uint8_t const VAL     = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(VAL, sp.a);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(VAL, sp.a);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpLogSaveableParams<TensorType>, D>
{
  using Type       = ml::OpLogSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;
  static uint8_t const AXIS    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE    = 1;
  static uint8_t const FILL_VALUE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(FILL_VALUE, sp.fill_value);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  static uint8_t const ERR_SIG_1 = 2;
  static uint8_t const ERR_SIG_2 = 3;

  static uint8_t const FWD_IN_SHAPE_1      = 4;
  static uint8_t const FWD_IN_SHAPE_2      = 5;
  static uint8_t const OUT_VIEW_TENSOR     = 6;
  static uint8_t const FWD_IN1_VIEW_TENSOR = 7;
  static uint8_t const FWD_IN2_VIEW_TENSOR = 8;

  static uint8_t const BACK_IN_SHAPE_1      = 9;
  static uint8_t const BACK_IN_SHAPE_2      = 10;
  static uint8_t const BACK_IN1_VIEW_TENSOR = 11;
  static uint8_t const BACK_IN2_VIEW_TENSOR = 12;
  static uint8_t const ERR_SIG_VIEW_TENSOR  = 13;
  static uint8_t const ERR1                 = 14;
  static uint8_t const ERR2                 = 15;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(15);
    map.Append(OP_CODE, sp.op_type);
    map.Append(ERR_SIG_1, sp.error_signal_1);
    map.Append(ERR_SIG_2, sp.error_signal_2);
    map.Append(FWD_IN_SHAPE_1, sp.fwd_input_shape_1);
    map.Append(FWD_IN_SHAPE_2, sp.fwd_input_shape_2);
    map.Append(OUT_VIEW_TENSOR, sp.output_view_tensor);
    map.Append(FWD_IN1_VIEW_TENSOR, sp.fwd_in1_view_tensor);
    map.Append(FWD_IN2_VIEW_TENSOR, sp.fwd_in2_view_tensor);
    map.Append(BACK_IN_SHAPE_1, sp.back_input_shape_1);
    map.Append(BACK_IN_SHAPE_2, sp.back_input_shape_2);
    map.Append(BACK_IN1_VIEW_TENSOR, sp.back_in1_view_tensor);
    map.Append(BACK_IN2_VIEW_TENSOR, sp.back_in2_view_tensor);
    map.Append(ERR_SIG_VIEW_TENSOR, sp.err_sig_view_tensor);
    map.Append(ERR1, sp.err1);
    map.Append(ERR2, sp.err2);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(ERR_SIG_1, sp.error_signal_1);
    map.ExpectKeyGetValue(ERR_SIG_2, sp.error_signal_2);
    map.ExpectKeyGetValue(FWD_IN_SHAPE_1, sp.fwd_input_shape_1);
    map.ExpectKeyGetValue(FWD_IN_SHAPE_2, sp.fwd_input_shape_2);
    map.ExpectKeyGetValue(OUT_VIEW_TENSOR, sp.output_view_tensor);
    map.ExpectKeyGetValue(FWD_IN1_VIEW_TENSOR, sp.fwd_in1_view_tensor);
    map.ExpectKeyGetValue(FWD_IN2_VIEW_TENSOR, sp.fwd_in2_view_tensor);
    map.ExpectKeyGetValue(BACK_IN_SHAPE_1, sp.back_input_shape_1);
    map.ExpectKeyGetValue(BACK_IN_SHAPE_2, sp.back_input_shape_2);
    map.ExpectKeyGetValue(BACK_IN1_VIEW_TENSOR, sp.back_in1_view_tensor);
    map.ExpectKeyGetValue(BACK_IN2_VIEW_TENSOR, sp.back_in2_view_tensor);
    map.ExpectKeyGetValue(ERR_SIG_VIEW_TENSOR, sp.err_sig_view_tensor);
    map.ExpectKeyGetValue(ERR1, sp.err1);
    map.ExpectKeyGetValue(ERR2, sp.err2);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPool1DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaxPool1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const KERNEL_SIZE = 2;
  static uint8_t const STRIDE_SIZE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMaxPool2DSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMaxPool2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const KERNEL_SIZE = 2;
  static uint8_t const STRIDE_SIZE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(STRIDE_SIZE, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(STRIDE_SIZE, sp.stride_size);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::OpMeanSquareErrorSaveableParams<TensorType>, D>
{
  using Type       = ml::OpMeanSquareErrorSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE    = 1;
  static uint8_t const WEIGHTINGS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(WEIGHTINGS, sp.weightings);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE        = 1;
  static uint8_t const OUTPUT_PRESENT = 2;
  static uint8_t const OUTPUT         = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);
    map.Append(OP_CODE, sp.op_type);
    if (sp.output)
    {
      map.Append(OUTPUT_PRESENT, true);
      map.Append(OUTPUT, *(sp.output));
    }
    else
    {
      map.Append(OUTPUT_PRESENT, false);
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    bool has_weights = true;
    map.ExpectKeyGetValue(OUTPUT_PRESENT, has_weights);
    if (has_weights)
    {
      TensorType output;
      map.ExpectKeyGetValue(OUTPUT, output);
      sp.output = std::make_shared<TensorType>(output);
    }
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

  static uint8_t const OP_CODE      = 1;
  static uint8_t const LOWER_BOUND  = 2;
  static uint8_t const UPPER_BOUND  = 3;
  static uint8_t const RANDOM_SEED  = 4;
  static uint8_t const BUFFER       = 5;
  static uint8_t const INDEX        = 6;
  static uint8_t const RANDOM_VALUE = 7;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(7);
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE   = 1;
  static uint8_t const NEW_SHAPE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(NEW_SHAPE, sp.new_shape);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(NEW_SHAPE, sp.new_shape);
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
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

  static uint8_t const OP_CODE = 1;
  static uint8_t const AXIS    = 2;
  static uint8_t const AXES    = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.axis);
    map.Append(AXES, sp.axes);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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

  static uint8_t const OP_CODE          = 1;
  static uint8_t const TRANSPOSE_VECTOR = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);
    map.Append(TRANSPOSE_VECTOR, sp.transpose_vector);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(TRANSPOSE_VECTOR, sp.transpose_vector);
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

  static uint8_t const REGULARISATION_TYPE = 3;
  static uint8_t const REGULARISATION_RATE = 4;

  static uint8_t const HAS_GRADIENT          = 5;
  static uint8_t const GRADIENT_ACCUMULATION = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    // serialize parent class first
    auto base_pointer = static_cast<ml::OpPlaceholderSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);

    map.Append(OP_CODE, sp.op_type);

    // first set the regulariser type
    map.Append(REGULARISATION_TYPE, static_cast<uint8_t>(sp.regularisation_type));
    map.Append(REGULARISATION_RATE, sp.regularisation_rate);

    //
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
    auto base_pointer = static_cast<ml::OpPlaceholderSaveableParams<TensorType> *>(&sp);
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

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OUTPUT_CHANNELS, sp.output_channels);
    map.Append(STRIDE_SIZE, sp.stride_size);
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

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OUTPUT_CHANNELS, sp.output_channels);
    map.Append(STRIDE_SIZE, sp.stride_size);
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

  static uint8_t const SUB_GRAPH        = 1;
  static uint8_t const OP_CODE          = 2;
  static uint8_t const IN_SIZE          = 3;
  static uint8_t const OUT_SIZE         = 4;
  static uint8_t const TIME_DISTRIBUTED = 5;

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
    map.Append(TIME_DISTRIBUTED, sp.time_distributed);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.in_size);
    map.ExpectKeyGetValue(OUT_SIZE, sp.out_size);
    map.ExpectKeyGetValue(TIME_DISTRIBUTED, sp.time_distributed);
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
 * serializer for self attention layer saveable params
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
    map.Append(N_HEADS, sp.n_heads);
    map.Append(MODEL_DIM, sp.model_dim);
    map.Append(VALUE_DIM, sp.value_dim);
    map.Append(KEY_DIM, sp.key_dim);
    map.Append(DROPOUT, sp.dropout);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(N_HEADS, sp.n_heads);
    map.ExpectKeyGetValue(MODEL_DIM, sp.model_dim);
    map.ExpectKeyGetValue(VALUE_DIM, sp.value_dim);
    map.ExpectKeyGetValue(KEY_DIM, sp.key_dim);
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
 * serializer for self attention layer saveable params
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

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(KEY_DIM, sp.key_dim);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KEY_DIM, sp.key_dim);
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
 * serializer for self attention layer saveable params
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

}  // namespace serializers
}  // namespace fetch
