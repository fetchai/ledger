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

#include "ml/meta/ml_type_traits.hpp"

namespace fetch {
namespace ml {

/**
 * Generic container for all the saveable params of an op.
 */
struct OpsSaveableParams
{
  OpsSaveableParams() = default;

  virtual ~OpsSaveableParams() = default;

  fetch::ml::OpType op_type     = OpType::NONE;
  bool              is_training = true;
};

////////////////////////////
/// FORWARD DECLARATIONS ///
////////////////////////////

template <class TensorType>
struct OpWeightsSaveableParams;

template <class TensorType>
struct NodeSaveableParams
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  std::string                        name = "";
  OpType                             operation_type       = OpType::NONE;
  std::shared_ptr<OpsSaveableParams> op_save_params;

  NodeSaveableParams() = default;
};

template <class TensorType>
struct GraphSaveableParams
{
  using DataType            = typename TensorType::Type;
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::GRAPH;

  virtual ~GraphSaveableParams() = default;

  std::vector<std::pair<std::string, std::vector<std::string>>>                    connections;
  std::unordered_map<std::string, std::shared_ptr<NodeSaveableParams<TensorType>>> nodes;

  uint8_t graph_state;
};

template <class TensorType>
struct SubGraphSaveableParams : public GraphSaveableParams<TensorType>, public OpsSaveableParams
{
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::SUBGRAPH;

  std::vector<std::string> input_node_names;
  std::string              output_node_name;
};

///////////////////////////////
/// ALL OPS SAVEABLE PARAMS ///
///////////////////////////////

/**
 * Saveable parameters for Abs op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct OpAbsSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::OP_ABS;
};

/**
 * Saveable parameters for Add op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct OpAddSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType                  op_type = OpType::OP_ADD;
  std::vector<fetch::math::SizeType> axes{};
};

/**
 * Saveable parameters for Concatenate op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct OpConcatenateSaveableParams : public OpsSaveableParams
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::OP_CONCATENATE;
};

/**
 * Saveable parameters for Conv1D op
 * @tparam TensorType
 */
template <class TensorType>
struct OpConvolution1DSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType     op_type     = OpType::OP_CONVOLUTION_1D;
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
};

/**
 * Saveable parameters for Conv2D op
 * @tparam TensorType
 */
template <class TensorType>
struct OpConvolution2DSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType     op_type     = OpType::OP_CONVOLUTION_2D;
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
};

/**
 * Saveable parameters for Cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct OpCrossEntropyLossSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::LOSS_CROSS_ENTROPY;
};

/**
 * Saveable parameters for Divide op
 * @tparam TensorType
 */
template <class TensorType>
struct OpDivideSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::OP_DIVIDE;
};

/**
 * Saveable parameters for Dropout op
 * @tparam TensorType
 */
template <class TensorType>
struct OpDropoutSaveableParams : public OpsSaveableParams
{
  using DataType                = typename TensorType::Type;
  using SizeType                = typename TensorType::SizeType;
  fetch::ml::OpType     op_type = OpType::OP_DROPOUT;
  DataType              probability{};
  TensorType            drop_values;
  SizeType              random_seed{};
  std::vector<uint64_t> buffer{};
  uint64_t              index = fetch::math::numeric_max<uint64_t>();
};

/**
 * Saveable parameters for Elu op
 * @tparam TensorType
 */
template <class TensorType>
struct OpEluSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::OP_ELU;
  using DataType            = typename TensorType::Type;
  DataType a;
};

/**
 * Saveable parameters for Elu op
 * @tparam TensorType
 */
template <class TensorType>
struct OpGeluSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::OP_GELU;
  using DataType            = typename TensorType::Type;
};

/**
 * Saveable parameters for Embeddings op
 * @tparam TensorType
 */
template <class TensorType>
struct OpEmbeddingsSaveableParams : public OpWeightsSaveableParams<TensorType>
{
  fetch::ml::OpType                  op_type = OpType::OP_EMBEDDINGS;
  std::vector<fetch::math::SizeType> updated_rows;
  std::vector<fetch::math::SizeType> trailing_indices1 = {0, 0};
  std::vector<fetch::math::SizeType> trailing_indices2 = {0};
};

/**
 * Saveable parameters for Exp op
 * @tparam TensorType
 */
template <class TensorType>
struct OpExpSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::OP_EXP;
};

/**
 * Saveable parameters for Flatten op
 * @tparam TensorType
 */
template <class TensorType>
struct OpFlattenSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType                  op_type = OpType::OP_FLATTEN;
  std::vector<fetch::math::SizeType> input_shape;
};

template <class TensorType>
struct LayerConvolution1DSaveableParams : SubGraphSaveableParams<TensorType>
{
  fetch::ml::OpType op_type = OpType::LAYER_CONVOLUTION_1D;

  using SizeType = typename TensorType::SizeType;

  SizeType kernel_size;
  SizeType input_channels;
  SizeType output_channels;
  SizeType stride_size;
};

template <class TensorType>
struct LayerConvolution2DSaveableParams : SubGraphSaveableParams<TensorType>
{
  fetch::ml::OpType op_type = OpType::LAYER_CONVOLUTION_2D;

  using SizeType = typename TensorType::SizeType;

  SizeType kernel_size;
  SizeType input_channels;
  SizeType output_channels;
  SizeType stride_size;
};

/**
 * Saveable parameters for FullyConnectedLayer
 * @tparam TensorType
 */
template <class TensorType>
struct LayerFullyConnectedSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType                     = typename TensorType::SizeType;
  fetch::ml::OpType op_type          = OpType::LAYER_FULLY_CONNECTED;
  SizeType          in_size          = fetch::math::numeric_max<SizeType>();
  SizeType          out_size         = fetch::math::numeric_max<SizeType>();
  bool              time_distributed = false;
};

/**
 * Saveable parameters for LayerNorm op
 * @tparam TensorType
 */
template <class TensorType>
struct OpLayerNormSaveableParams : public OpsSaveableParams
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::OP_LAYER_NORM;

  DataType   epsilon;
  SizeType   axis;
  TensorType prev_input;
  TensorType cached_inv_sqrt_var;
  TensorType cached_output;
};

/**
 * Saveable parameters for Slice op
 * @tparam TensorType
 */
template <class TensorType>
struct OpSliceSaveableParams : public OpsSaveableParams
{
  using SizeType = typename TensorType::SizeType;

  std::vector<SizeType> axes;
  std::vector<SizeType> indices;
  SizeType              axis;
  SizeType              index;

  fetch::ml::OpType op_type = OpType::OP_SLICE;
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct OpLeakyReluSaveableParams : public OpsSaveableParams
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType op_type = OpType::OP_LEAKY_RELU;
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct OpLeakyReluOpSaveableParams : public OpsSaveableParams
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType op_type = OpType::OP_LEAKY_RELU_OP;
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct OpLogSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_LOG;
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct OpLogSigmoidSaveableParams : public OpsSaveableParams
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType op_type = OpType::OP_LOGSIGMOID;
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct OpLogSoftmaxSaveableParams : public OpsSaveableParams
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::OP_LOGSOFTMAX;
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct OpMaskFillSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType         op_type    = OpType::OP_MASK_FILL;
  typename TensorType::Type fill_value = fetch::math::numeric_max<typename TensorType::Type>();
};

template <class TensorType>
struct OpMatrixMultiplySaveableParams : public OpsSaveableParams
{
  using SizeType   = fetch::math::SizeType;
  using SizeVector = std::vector<SizeType>;

  fetch::ml::OpType op_type = OpType::OP_MATRIX_MULTIPLY;

  TensorType error_signal_1;
  TensorType error_signal_2;

  // forward pass
  SizeVector fwd_input_shape_1{};
  SizeVector fwd_input_shape_2{};
  TensorType output_view_tensor;
  TensorType fwd_in1_view_tensor;
  TensorType fwd_in2_view_tensor;

  // backward pass
  SizeVector back_input_shape_1{};
  SizeVector back_input_shape_2{};
  TensorType back_in1_view_tensor;
  TensorType back_in2_view_tensor;
  TensorType err_sig_view_tensor;
  TensorType err1;
  TensorType err2;
};

template <class TensorType>
struct OpMaxPool1DSaveableParams : public OpsSaveableParams
{
  fetch::math::SizeType kernel_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type     = OpType::OP_MAX_POOL_1D;
};

template <class TensorType>
struct OpMaxPool2DSaveableParams : public OpsSaveableParams
{
  fetch::math::SizeType kernel_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type     = OpType::OP_MAX_POOL_2D;
};

/**
 * Saveable parameters for MSE op
 * @tparam TensorType
 */
template <class TensorType>
struct OpMeanSquareErrorSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::LOSS_MEAN_SQUARE_ERROR;
  TensorType        weightings;
};

/**
 * Saveable parameters for Maximum op
 * @tparam TensorType
 */
template <class TensorType>
struct OpMaximumSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_MAXIMUM;
};

/**
 * Saveable parameters for Multiply op
 * @tparam TensorType
 */
template <class TensorType>
struct OpMultiplySaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_MULTIPLY;
};

/**
 * Saveable parameters for LayerNorm op
 * @tparam TensorType
 */
template <class TensorType>
struct LayerLayerNormSaveableParams : SubGraphSaveableParams<TensorType>
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::LAYER_LAYER_NORM;

  std::vector<SizeType> data_shape;
  SizeType              axis;
  DataType              epsilon;
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct LayerMultiHeadSaveableParams : public SubGraphSaveableParams<TensorType>
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::LAYER_MULTI_HEAD_ATTENTION;

  SizeType value_dim;
  SizeType key_dim;
  SizeType n_heads;
  SizeType model_dim;
  DataType dropout;
};

template <class TensorType>
struct OpPlaceholderSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType           op_type = OpType::OP_PLACEHOLDER;
  std::shared_ptr<TensorType> output;
};

/**
 * Saveable parameters for Layer PRelu saveable params
 * @tparam TensorType
 */
template <class TensorType>
struct LayerPReluSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::LAYER_PRELU;
};

template <class TensorType>
struct OpRandomisedReluSaveableParams : public OpsSaveableParams
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::OP_RANDOMISED_RELU;

  DataType              lower_bound  = fetch::math::numeric_max<DataType>();
  DataType              upper_bound  = fetch::math::numeric_max<DataType>();
  SizeType              random_seed  = fetch::math::numeric_max<SizeType>();
  std::vector<uint64_t> buffer       = {};
  uint64_t              index        = fetch::math::numeric_max<uint64_t>();
  DataType              random_value = fetch::math::numeric_max<DataType>();
};

/**
 * Saveable parameters for Relu op
 * @tparam TensorType
 */
template <class TensorType>
struct OpReluSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_RELU;
};

template <class TensorType>
struct OpReshapeSaveableParams : public OpsSaveableParams
{
  std::vector<fetch::math::SizeType> new_shape;
  fetch::ml::OpType                  op_type = OpType::OP_RESHAPE;
};

/**
 * Saveable parameters for Self Attention Layer
 * @tparam TensorType
 */
template <class TensorType>
struct LayerScaledDotProductAttentionSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  fetch::ml::OpType op_type = OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION;
  SizeType          key_dim = fetch::math::numeric_max<SizeType>();
  DataType          dropout = fetch::math::numeric_max<DataType>();
};

/**
 * Saveable parameters for Self Attention Layer
 * @tparam TensorType
 */
template <class TensorType>
struct LayerSelfAttentionEncoderSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  fetch::ml::OpType op_type             = OpType::LAYER_SELF_ATTENTION_ENCODER;
  SizeType          n_heads             = fetch::math::numeric_max<SizeType>();
  SizeType          model_dim           = fetch::math::numeric_max<SizeType>();
  SizeType          ff_dim              = fetch::math::numeric_max<SizeType>();
  DataType          residual_dropout    = fetch::math::numeric_max<DataType>();
  DataType          attention_dropout   = fetch::math::numeric_max<DataType>();
  DataType          feedforward_dropout = fetch::math::numeric_max<DataType>();
  DataType          epsilon             = fetch::math::function_tolerance<DataType>();
};

/**
 * Saveable parameters for Relu op
 * @tparam TensorType
 */
template <class TensorType>
struct OpSigmoidSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SIGMOID;
};

/**
 * Saveable parameters for Self Attention Layer
 * @tparam TensorType
 */
template <class TensorType>
struct LayerSkipGramSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType                   = typename TensorType::SizeType;
  fetch::ml::OpType op_type        = OpType::LAYER_SKIP_GRAM;
  std::string       embed_in       = "";
  SizeType          in_size        = fetch::math::numeric_max<SizeType>();
  SizeType          out_size       = fetch::math::numeric_max<SizeType>();
  SizeType          vocab_size     = fetch::math::numeric_max<SizeType>();
  SizeType          embedding_size = fetch::math::numeric_max<SizeType>();
};

template <class TensorType>
struct OpSoftmaxSaveableParams : public OpsSaveableParams
{
  fetch::math::SizeType              axis = fetch::math::numeric_max<fetch::math::SizeType>();
  std::vector<fetch::math::SizeType> axes{};
  fetch::ml::OpType                  op_type = OpType::OP_SOFTMAX;
};

/**
 * Saveable parameters for Softmax cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct OpSoftmaxCrossEntropySaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::LOSS_SOFTMAX_CROSS_ENTROPY;
};

/**
 * Saveable parameters for Softmax cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct OpSQRTSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SQRT;
};

/**
 * Saveable parameters for subtract op
 * @tparam TensorType
 */
template <class TensorType>
struct OpSubtractSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SUBTRACT;
};

template <class TensorType>
struct OpSwitchSaveableParams : public OpsSaveableParams
{
  fetch::ml::OpType op_type = OpType::OP_SWITCH;
};

/**
 * Saveable parameters for subtract op
 * @tparam TensorType
 */
template <class TensorType>
struct OpTanhSaveableParams : public OpsSaveableParams
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_TANH;
};

template <class TensorType>
struct OpTransposeSaveableParams : public OpsSaveableParams
{
  std::vector<fetch::math::SizeType> transpose_vector;
  fetch::ml::OpType                  op_type = OpType::OP_TRANSPOSE;
};

}  // namespace ml
}  // namespace fetch
