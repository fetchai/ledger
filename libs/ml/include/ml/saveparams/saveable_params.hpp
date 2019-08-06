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

//#include "core/serializers/main_serializer.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/regularisers/regularisation.hpp"

namespace fetch {
namespace ml {

/**
 * Generic container for all the saveable params of an op.
 */
struct SaveableParamsInterface
{
  SaveableParamsInterface() = default;

  explicit SaveableParamsInterface(OpType op_type)
    : op_type(op_type)
  {}
  virtual ~SaveableParamsInterface() = default;

  fetch::ml::OpType op_type = OpType::NONE;
};

////////////////////////////
/// FORWARD DECLARATIONS ///
////////////////////////////

template <class TensorType>
struct WeightsSaveableParams;

template <class TensorType>
struct GraphSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::GRAPH;

  std::vector<std::pair<std::string, std::vector<std::string>>>             connections;
  std::unordered_map<std::string, std::shared_ptr<SaveableParamsInterface>> nodes;

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
};

template <class TensorType>
struct NodeSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  std::string name = "";
  TensorType  cached_output{};
  uint8_t     cached_output_status = fetch::math::numeric_max<uint8_t>();
  OpType      operation_type       = OpType::NONE;
  std::shared_ptr<SaveableParamsInterface> op_save_params =
      std::make_shared<SaveableParamsInterface>();

  NodeSaveableParams() = default;

  NodeSaveableParams(std::string in_name, TensorType const &in_cached_output,
                     uint8_t in_cached_output_status, OpType in_operation_type,
                     std::shared_ptr<SaveableParamsInterface> in_op)
    : name(std::move(in_name))
    , cached_output(in_cached_output)
    , cached_output_status(in_cached_output_status)
    , operation_type(in_operation_type)
    , op_save_params(std::move(in_op))
  {}
};

template <class TensorType>
struct SubGraphSaveableParams : public GraphSaveableParams<TensorType>
{
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::SUBGRAPH;

  std::vector<std::string> input_node_names;
  std::string              output_node_name;

  SubGraphSaveableParams() = default;

  explicit SubGraphSaveableParams(OpType operation_type)
    : GraphSaveableParams<TensorType>(operation_type)
    , op_type(operation_type)
  {}

  SubGraphSaveableParams &operator=(SubGraphSaveableParams const &sgsp)
  {
    input_node_names = sgsp.input_node_names;
    output_node_name = sgsp.output_node_name;
    return *this;
  }
};

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
    : SaveableParamsInterface(OpType::OP_ABS)
  {}
  fetch::ml::OpType op_type = OpType::OP_ABS;
};

/**
 * Saveable parameters for Add op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct AddSaveableParams : public SaveableParamsInterface
{
  AddSaveableParams()
    : SaveableParamsInterface(OpType::OP_ADD)
  {}
  fetch::ml::OpType op_type = OpType::OP_ADD;
};

/**
 * Saveable parameters for Concatenate op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct ConcatenateSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::OP_CONCATENATE;

  ConcatenateSaveableParams()
    : SaveableParamsInterface(OpType::OP_CONCATENATE)
  {}
};

/**
 * Saveable parameters for Conv1D op
 * @tparam TensorType
 */
template <class TensorType>
struct Convolution1DSaveableParams : public SaveableParamsInterface
{
  Convolution1DSaveableParams()
    : SaveableParamsInterface(OpType::OP_CONVOLUTION_1D)
  {}
  fetch::ml::OpType     op_type     = OpType::OP_CONVOLUTION_1D;
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
};

/**
 * Saveable parameters for Conv2D op
 * @tparam TensorType
 */
template <class TensorType>
struct Convolution2DSaveableParams : public SaveableParamsInterface
{
  Convolution2DSaveableParams()
    : SaveableParamsInterface(OpType::OP_CONVOLUTION_2D)
  {}
  fetch::ml::OpType     op_type     = OpType::OP_CONVOLUTION_2D;
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
};

/**
 * Saveable parameters for Cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct CrossEntropyLossSaveableParams : public SaveableParamsInterface
{
  CrossEntropyLossSaveableParams()
    : SaveableParamsInterface(OpType::OP_CROSS_ENTROPY_LOSS)
  {}
  fetch::ml::OpType op_type = OpType::OP_CROSS_ENTROPY_LOSS;
};

/**
 * Saveable parameters for Divide op
 * @tparam TensorType
 */
template <class TensorType>
struct DivideSaveableParams : public SaveableParamsInterface
{
  DivideSaveableParams()
    : SaveableParamsInterface(OpType::OP_DIVIDE)
  {}
  fetch::ml::OpType op_type = OpType::OP_DIVIDE;
};

/**
 * Saveable parameters for Dropout op
 * @tparam TensorType
 */
template <class TensorType>
struct DropoutSaveableParams : public SaveableParamsInterface
{
  DropoutSaveableParams()
    : SaveableParamsInterface(OpType::OP_DROPOUT)
  {}

  using DataType                = typename TensorType::Type;
  using SizeType                = typename TensorType::SizeType;
  fetch::ml::OpType     op_type = OpType::OP_DROPOUT;
  SizeType              random_seed{};
  DataType              probability{};
  std::vector<uint64_t> buffer{};
  uint64_t              index = fetch::math::numeric_max<uint64_t>();
};

/**
 * Saveable parameters for Elu op
 * @tparam TensorType
 */
template <class TensorType>
struct EluSaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType op_type = OpType::OP_ELU;
  using DataType            = typename TensorType::Type;
  DataType a;

  EluSaveableParams()
    : SaveableParamsInterface(OpType::OP_ELU)
  {}
};

/**
 * Saveable parameters for Embeddings op
 * @tparam TensorType
 */
template <class TensorType>
struct EmbeddingsSaveableParams : public WeightsSaveableParams<TensorType>
{
  EmbeddingsSaveableParams()
    : WeightsSaveableParams<TensorType>(OpType::OP_EMBEDDINGS)
  {}
  fetch::ml::OpType op_type = OpType::OP_EMBEDDINGS;
};

/**
 * Saveable parameters for Exp op
 * @tparam TensorType
 */
template <class TensorType>
struct ExpSaveableParams : public SaveableParamsInterface
{
  ExpSaveableParams()
    : SaveableParamsInterface(OpType::OP_EXP)
  {}
  fetch::ml::OpType op_type = OpType::OP_EXP;
};

/**
 * Saveable parameters for Flatten op
 * @tparam TensorType
 */
template <class TensorType>
struct FlattenSaveableParams : public SaveableParamsInterface
{
  FlattenSaveableParams()
    : SaveableParamsInterface(OpType::OP_FLATTEN)
  {}
  fetch::ml::OpType op_type = OpType::OP_FLATTEN;
};

template <class TensorType>
struct ConvolutionLayer1DSaveableParams : SubGraphSaveableParams<TensorType>
{
  ConvolutionLayer1DSaveableParams() = default;

  fetch::ml::OpType op_type = OpType::LAYER_CONVOLUTION_1D;

  using SizeType = typename TensorType::SizeType;

  SizeType kernel_size;
  SizeType input_channels;
  SizeType output_channels;
  SizeType stride_size;
};

template <class TensorType>
struct ConvolutionLayer2DSaveableParams : SubGraphSaveableParams<TensorType>
{
  ConvolutionLayer2DSaveableParams() = default;

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
struct FullyConnectedSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType             = typename TensorType::SizeType;
  fetch::ml::OpType op_type  = OpType::LAYER_FULLY_CONNECTED;
  SizeType          in_size  = fetch::math::numeric_max<SizeType>();
  SizeType          out_size = fetch::math::numeric_max<SizeType>();

  FullyConnectedSaveableParams()
    : SubGraphSaveableParams<TensorType>(OpType::LAYER_FULLY_CONNECTED)
  {}
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct LayerNormSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::OP_LAYER_NORM;

  DataType   epsilon;
  SizeType   axis;
  TensorType prev_input;
  TensorType cached_inv_sqrt_var;
  TensorType cached_output;

  LayerNormSaveableParams()
    : SaveableParamsInterface(OpType::LAYER_LAYER_NORM)
  {}
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct LeakyReluSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType op_type = OpType::OP_LEAKY_RELU;

  LeakyReluSaveableParams()
    : SaveableParamsInterface(OpType::OP_LEAKY_RELU)
  {}
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct LeakyReluOpSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType op_type = OpType::OP_LEAKY_RELU_OP;

  LeakyReluOpSaveableParams()
    : SaveableParamsInterface(OpType::OP_LEAKY_RELU_OP)
  {}
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct LogSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_LOG;

  LogSaveableParams()
    : SaveableParamsInterface(OpType::OP_LOG)
  {}
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
  fetch::ml::OpType op_type = OpType::OP_LOGSIGMOID;

  LogSigmoidSaveableParams()
    : SaveableParamsInterface(OpType::OP_LOGSIGMOID)
  {}
};

/**
 * Saveable parameters for Log op
 * @tparam TensorType
 */
template <class TensorType>
struct LogSoftmaxSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::OP_LOGSOFTMAX;

  LogSoftmaxSaveableParams()
    : SaveableParamsInterface(OpType::OP_LOGSOFTMAX)
  {}
};

template <class TensorType>
struct MatrixMultiplySaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType op_type = OpType::OP_MATRIX_MULTIPLY;

  MatrixMultiplySaveableParams()
    : SaveableParamsInterface(OpType::OP_MATRIX_MULTIPLY)
  {}
};

template <class TensorType>
struct MaxPool1DSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType kernel_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type     = OpType::OP_MAX_POOL_1D;

  MaxPool1DSaveableParams()
    : SaveableParamsInterface(OpType::OP_MAX_POOL_1D)
  {}
};

template <class TensorType>
struct MaxPool2DSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType kernel_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type     = OpType::OP_MAX_POOL_2D;

  MaxPool2DSaveableParams()
    : SaveableParamsInterface(OpType::OP_MAX_POOL_2D)
  {}
};

/**
 * Saveable parameters for MSE op
 * @tparam TensorType
 */
template <class TensorType>
struct MeanSquareErrorSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_MEAN_SQUARE_ERROR_LOSS;

  MeanSquareErrorSaveableParams()
    : SaveableParamsInterface(OpType::OP_MEAN_SQUARE_ERROR_LOSS)
  {}
};

/**
 * Saveable parameters for Maximum op
 * @tparam TensorType
 */
template <class TensorType>
struct MaximumSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_MAXIMUM;

  MaximumSaveableParams()
    : SaveableParamsInterface(OpType::OP_MAXIMUM)
  {}
};

/**
 * Saveable parameters for Multiply op
 * @tparam TensorType
 */
template <class TensorType>
struct MultiplySaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_MULTIPLY;

  MultiplySaveableParams()
    : SaveableParamsInterface(OpType::OP_MULTIPLY)
  {}
};

/**
 * Saveable parameters for LeakyRelu op
 * @tparam TensorType
 */
template <class TensorType>
struct MultiHeadSaveableParams : public SubGraphSaveableParams<TensorType>
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::LAYER_MULTI_HEAD_ATTENTION;

  SizeType value_dim;
  SizeType key_dim;
  SizeType n_heads;
  SizeType model_dim;
  DataType dropout;

  MultiHeadSaveableParams()
    : SubGraphSaveableParams<TensorType>(OpType::LAYER_MULTI_HEAD_ATTENTION)
  {}
};

template <class TensorType>
struct PlaceholderSaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType           op_type = OpType::OP_PLACEHOLDER;
  std::shared_ptr<TensorType> output;

  PlaceholderSaveableParams()
    : SaveableParamsInterface(OpType::OP_PLACEHOLDER)
  {}
};

/**
 * Saveable parameters for Flatten op
 * @tparam TensorType
 */
template <class TensorType>
struct PReluSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::LAYER_PRELU;

  PReluSaveableParams()
    : SubGraphSaveableParams<TensorType>(OpType::LAYER_PRELU)
  {}
};

template <class TensorType>
struct RandomisedReluSaveableParams : public SaveableParamsInterface
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

  RandomisedReluSaveableParams()
    : SaveableParamsInterface(OpType::OP_RANDOMISED_RELU)
  {}
};

/**
 * Saveable parameters for Relu op
 * @tparam TensorType
 */
template <class TensorType>
struct ReluSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_RELU;

  ReluSaveableParams()
    : SaveableParamsInterface(OpType::OP_RELU)
  {}
};

template <class TensorType>
struct ReshapeSaveableParams : public SaveableParamsInterface
{
  std::vector<fetch::math::SizeType> new_shape;
  fetch::ml::OpType                  op_type = OpType::OP_RESHAPE;

  ReshapeSaveableParams()
    : SaveableParamsInterface(OpType::OP_RESHAPE)
  {}
};

/**
 * Saveable parameters for Self Attention Layer
 * @tparam TensorType
 */
template <class TensorType>
struct SelfAttentionEncoderSaveableParams : SubGraphSaveableParams<TensorType>
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

  SelfAttentionEncoderSaveableParams()
    : SubGraphSaveableParams<TensorType>(OpType::LAYER_SELF_ATTENTION_ENCODER)
  {}
};

/**
 * Saveable parameters for Relu op
 * @tparam TensorType
 */
template <class TensorType>
struct SigmoidSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SIGMOID;

  SigmoidSaveableParams()
    : SaveableParamsInterface(OpType::OP_SIGMOID)
  {}
};

/**
 * Saveable parameters for Self Attention Layer
 * @tparam TensorType
 */
template <class TensorType>
struct SkipGramSaveableParams : SubGraphSaveableParams<TensorType>
{
  using SizeType             = typename TensorType::SizeType;
  fetch::ml::OpType op_type  = OpType::LAYER_SKIP_GRAM;
  std::string       embed_in = "";
  SizeType          in_size  = fetch::math::numeric_max<SizeType>();
  SizeType          out_size = fetch::math::numeric_max<SizeType>();

  SkipGramSaveableParams()
    : SubGraphSaveableParams<TensorType>(OpType::LAYER_SKIP_GRAM)
  {}
};

template <class TensorType>
struct SoftmaxSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::OP_SOFTMAX;

  SoftmaxSaveableParams()
    : SaveableParamsInterface(OpType::OP_SOFTMAX)
  {}
};

/**
 * Saveable parameters for Softmax cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct SoftmaxCrossEntropySaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SOFTMAX_CROSS_ENTROPY_LOSS;

  SoftmaxCrossEntropySaveableParams()
    : SaveableParamsInterface(OpType::OP_SOFTMAX_CROSS_ENTROPY_LOSS)
  {}
};

/**
 * Saveable parameters for Softmax cross entropy loss op
 * @tparam TensorType
 */
template <class TensorType>
struct SQRTSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SQRT;

  SQRTSaveableParams()
    : SaveableParamsInterface(OpType::OP_SQRT)
  {}
};

/**
 * Saveable parameters for subtract op
 * @tparam TensorType
 */
template <class TensorType>
struct SubtractSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_SUBTRACT;

  SubtractSaveableParams()
    : SaveableParamsInterface(OpType::OP_SUBTRACT)
  {}
};

/**
 * Saveable parameters for subtract op
 * @tparam TensorType
 */
template <class TensorType>
struct TanhSaveableParams : public SaveableParamsInterface
{
  using DataType            = typename TensorType::Type;
  fetch::ml::OpType op_type = OpType::OP_TANH;

  TanhSaveableParams()
    : SaveableParamsInterface(OpType::OP_TANH)
  {}
};

template <class TensorType>
struct TransposeSaveableParams : public SaveableParamsInterface
{
  std::vector<fetch::math::SizeType> transpose_vector;
  fetch::ml::OpType                  op_type = OpType::OP_TRANSPOSE;

  TransposeSaveableParams()
    : SaveableParamsInterface(OpType::OP_TRANSPOSE)
  {}
};

template <class TensorType>
struct WeightsSaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType                                                 op_type = OpType::OP_WEIGHTS;
  std::shared_ptr<TensorType>                                       output;
  std::shared_ptr<TensorType>                                       gradient_accumulation;
  std::shared_ptr<fetch::ml::regularisers::Regulariser<TensorType>> regulariser;
  typename TensorType::Type                                         regularisation_rate;

  WeightsSaveableParams()
    : SaveableParamsInterface(OpType::OP_WEIGHTS)
  {}

  explicit WeightsSaveableParams(OpType operation_type)
    : SaveableParamsInterface(operation_type)
  {}
};

}  // namespace ml
}  // namespace fetch
