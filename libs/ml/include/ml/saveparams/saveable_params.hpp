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
struct ConvolutionLayer1DSaveableParams;

template <class TensorType>
struct ConvolutionLayer2DSaveableParams;

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
struct MaxPool1DSaveableParams;

template <class TensorType>
struct MaxPool2DSaveableParams;

template <class TensorType>
struct MeanSquareErrorSaveableParams;

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

  std::string name;
  TensorType  cached_output;
  uint8_t     cached_output_status;
  OpType      operation_type;
  std::shared_ptr<SaveableParamsInterface> op;

  NodeSaveableParams(std::string in_name, TensorType const &in_cached_output,
                     uint8_t in_cached_output_status, OpType in_operation_type, std::shared_ptr<SaveableParamsInterface> in_op)
    : name(std::move(in_name))
    , cached_output(in_cached_output)
    , cached_output_status(in_cached_output_status)
    , operation_type(in_operation_type)
    , op(std::move(in_op))
  {}
};

template <class TensorType>
struct SubGraphSaveableParams : GraphSaveableParams<TensorType>
{
  using SizeType            = typename TensorType::SizeType;
  fetch::ml::OpType op_type = OpType::SUBGRAPH;

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
  //
  //  template <typename S>
  //  friend void Serialize(S &serializer, SubGraphSaveableParams const &gsp)
  //  {
  //    Serialize(serializer, gsp.op_type);
  //
  //    // serialize parent class first
  //    auto base_pointer = static_cast<GraphSaveableParams<TensorType> const *>(&gsp);
  //    Serialize(serializer, *base_pointer);
  //
  //    serializer << gsp.input_node_names;
  //    serializer << gsp.output_node_name;
  //  }
  //
  //  template <typename S>
  //  friend void Deserialize(S &serializer, SubGraphSaveableParams &gsp)
  //  {
  //    Deserialize(serializer, gsp.op_type);
  //
  //    // deserialize parent class first
  //    auto base_pointer = static_cast<GraphSaveableParams<TensorType> *>(&gsp);
  //    Deserialize(serializer, *base_pointer);
  //
  //    serializer >> gsp.input_node_names;
  //    serializer >> gsp.output_node_name;
  //  }
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
    : SaveableParamsInterface(OpType::ABS)
  {}
  fetch::ml::OpType op_type = OpType::ABS;
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
  fetch::ml::OpType op_type = OpType::ADD;
};

/**
 * Saveable parameters for Concatenate op (only includes the descriptor)
 * @tparam TensorType
 */
template <class TensorType>
struct ConcatenateSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::CONCATENATE;

  ConcatenateSaveableParams()
    : SaveableParamsInterface(OpType::CONCATENATE)
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
    : SaveableParamsInterface(OpType::CONVOLUTION_1D)
  {}
  fetch::ml::OpType     op_type     = OpType::CONVOLUTION_1D;
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
    : SaveableParamsInterface(OpType::CONVOLUTION_2D)
  {}
  fetch::ml::OpType     op_type     = OpType::CONVOLUTION_2D;
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
    : SaveableParamsInterface(OpType::CROSS_ENTROPY_LOSS)
  {}
  fetch::ml::OpType op_type = OpType::CROSS_ENTROPY_LOSS;
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
  fetch::ml::OpType op_type = OpType::DIVIDE;
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

  using DataType                = typename TensorType::Type;
  using SizeType                = typename TensorType::SizeType;
  fetch::ml::OpType     op_type = OpType::DROPOUT;
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
  fetch::ml::OpType op_type = OpType::ELU;
  using DataType            = typename TensorType::Type;
  DataType a;

  EluSaveableParams()
    : SaveableParamsInterface(OpType::ELU)
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
    : WeightsSaveableParams<TensorType>(OpType::EMBEDDINGS)
  {}
  fetch::ml::OpType op_type = OpType::EMBEDDINGS;
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
  fetch::ml::OpType op_type = OpType::EXP;
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
  fetch::ml::OpType op_type = OpType::FLATTEN;
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
 * Saveable parameters for Flatten op
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
struct LeakyReluSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  DataType          a;
  fetch::ml::OpType op_type = OpType::LEAKY_RELU;

  LeakyReluSaveableParams()
    : SaveableParamsInterface(OpType::LEAKY_RELU)
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
  fetch::ml::OpType op_type = OpType::LEAKY_RELU_OP;

  LeakyReluOpSaveableParams()
    : SaveableParamsInterface(OpType::LEAKY_RELU_OP)
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
  fetch::ml::OpType op_type = OpType::LOG;

  LogSaveableParams()
    : SaveableParamsInterface(OpType::LOG)
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
  fetch::ml::OpType op_type = OpType::LOGSIGMOID;

  LogSigmoidSaveableParams()
    : SaveableParamsInterface(OpType::LOGSIGMOID)
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
  fetch::ml::OpType     op_type = OpType::LOGSOFTMAX;

  LogSoftmaxSaveableParams()
    : SaveableParamsInterface(OpType::LOGSOFTMAX)
  {}
};

template <class TensorType>
struct MatrixMultiplySaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType op_type = OpType::MATRIX_MULTIPLY;

  MatrixMultiplySaveableParams()
    : SaveableParamsInterface(OpType::MATRIX_MULTIPLY)
  {}
};

template <class TensorType>
struct MaxPool1DSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType kernel_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type     = OpType::MAX_POOL_1D;

  MaxPool1DSaveableParams()
    : SaveableParamsInterface(OpType::MAX_POOL_1D)
  {}
};

template <class TensorType>
struct MaxPool2DSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType kernel_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::math::SizeType stride_size = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type     = OpType::MAX_POOL_2D;

  MaxPool2DSaveableParams()
    : SaveableParamsInterface(OpType::MAX_POOL_2D)
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
  fetch::ml::OpType op_type = OpType::MEAN_SQUARE_ERROR_LOSS;

  MeanSquareErrorSaveableParams()
    : SaveableParamsInterface(OpType::MEAN_SQUARE_ERROR_LOSS)
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
  fetch::ml::OpType op_type = OpType::MAXIMUM;

  MaximumSaveableParams()
    : SaveableParamsInterface(OpType::MAXIMUM)
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
  fetch::ml::OpType op_type = OpType::MULTIPLY;

  MultiplySaveableParams()
    : SaveableParamsInterface(OpType::MULTIPLY)
  {}
};

template <class TensorType>
struct PlaceholderSaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType           op_type = OpType::PLACEHOLDER;
  std::shared_ptr<TensorType> output;

  PlaceholderSaveableParams()
    : SaveableParamsInterface(OpType::PLACEHOLDER)
  {}
};

template <class TensorType>
struct RandomisedReluSaveableParams : public SaveableParamsInterface
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

  fetch::ml::OpType op_type = OpType::RANDOMISED_RELU;

  DataType              lower_bound  = fetch::math::numeric_max<DataType>();
  DataType              upper_bound  = fetch::math::numeric_max<DataType>();
  SizeType              random_seed  = fetch::math::numeric_max<SizeType>();
  std::vector<uint64_t> buffer       = {};
  uint64_t              index        = fetch::math::numeric_max<uint64_t>();
  DataType              random_value = fetch::math::numeric_max<DataType>();

  RandomisedReluSaveableParams()
    : SaveableParamsInterface(OpType::RANDOMISED_RELU)
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
  fetch::ml::OpType op_type = OpType::RELU;

  ReluSaveableParams()
    : SaveableParamsInterface(OpType::RELU)
  {}
};

template <class TensorType>
struct ReshapeSaveableParams : public SaveableParamsInterface
{
  std::vector<fetch::math::SizeType> new_shape;
  fetch::ml::OpType                  op_type = OpType::RESHAPE;

  ReshapeSaveableParams()
    : SaveableParamsInterface(OpType::RESHAPE)
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
  fetch::ml::OpType op_type = OpType::SIGMOID;

  SigmoidSaveableParams()
    : SaveableParamsInterface(OpType::SIGMOID)
  {}
};

template <class TensorType>
struct SoftmaxSaveableParams : public SaveableParamsInterface
{
  fetch::math::SizeType axis    = fetch::math::numeric_max<fetch::math::SizeType>();
  fetch::ml::OpType     op_type = OpType::SOFTMAX;

  SoftmaxSaveableParams()
    : SaveableParamsInterface(OpType::SOFTMAX)
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
  fetch::ml::OpType op_type = OpType::SOFTMAX_CROSS_ENTROPY_LOSS;

  SoftmaxCrossEntropySaveableParams()
    : SaveableParamsInterface(OpType::SOFTMAX_CROSS_ENTROPY_LOSS)
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
  fetch::ml::OpType op_type = OpType::SQRT;

  SQRTSaveableParams()
    : SaveableParamsInterface(OpType::SQRT)
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
  fetch::ml::OpType op_type = OpType::SUBTRACT;

  SubtractSaveableParams()
    : SaveableParamsInterface(OpType::SUBTRACT)
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
  fetch::ml::OpType op_type = OpType::TANH;

  TanhSaveableParams()
    : SaveableParamsInterface(OpType::TANH)
  {}
};

template <class TensorType>
struct TransposeSaveableParams : public SaveableParamsInterface
{
  std::vector<fetch::math::SizeType> transpose_vector;
  fetch::ml::OpType                  op_type = OpType::TRANSPOSE;

  TransposeSaveableParams()
    : SaveableParamsInterface(OpType::TRANSPOSE)
  {}
};

template <class TensorType>
struct WeightsSaveableParams : public SaveableParamsInterface
{
  fetch::ml::OpType                                                 op_type = OpType::WEIGHTS;
  std::shared_ptr<TensorType>                                       output;
  std::shared_ptr<TensorType>                                       gradient_accumulation;
  std::shared_ptr<fetch::ml::regularisers::Regulariser<TensorType>> regulariser;
  typename TensorType::Type                                         regularisation_rate;

  WeightsSaveableParams()
    : SaveableParamsInterface(OpType::WEIGHTS)
  {}

  explicit WeightsSaveableParams(OpType operation_type)
    : SaveableParamsInterface(operation_type)
  {}

  //  template <class S>
  //  friend void Deserialize(S &serializer, WeightsSaveableParams<TensorType> &sp)
  //  {
  //    Deserialize(serializer, sp.op_type);
  //    bool has_weights{};
  //    serializer >> has_weights;
  //    if (has_weights)
  //    {
  //      TensorType output_temp;
  //      serializer >> output_temp;
  //      sp.output = std::make_shared<TensorType>(output_temp);
  //    }
  //    int reg_type_int{};
  //    serializer >> reg_type_int;
  //    sp.regularisation_type = static_cast<fetch::ml::details::RegularisationType>(reg_type_int);
  //    serializer >> sp.regularisation_rate;
  //  }
};

}  // namespace ml
}  // namespace fetch
