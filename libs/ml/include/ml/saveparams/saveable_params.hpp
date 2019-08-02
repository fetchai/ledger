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

#include "core/serializers/main_serializer.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/regularisers/regularisation.hpp"

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
  uint16_t enum_val = 0;
  serializer >> enum_val;
  operation_type = static_cast<OpType>(enum_val);
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
    : op_type(op_type)
  {}
  virtual ~SaveableParamsInterface() = default;

  fetch::ml::OpType op_type = OpType::NONE;
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

///////////////////////////////////////////
/// OP SPECIFIC SERIALISATION FUNCTIONS ///
///////////////////////////////////////////

namespace {
template <typename S, class SP>
void SerializeImplementation(S &serializer, std::shared_ptr<SaveableParamsInterface> const &nsp)
{
  auto castnode = std::dynamic_pointer_cast<SP>(nsp);
  serializer << *castnode;
}

template <typename S, class SP>
void DeserializeImplementation(S &serializer, std::shared_ptr<SaveableParamsInterface> &nsp)
{
  auto castnode = std::dynamic_pointer_cast<SP>(nsp);
  serializer >> *castnode;
}
}  // namespace

template <typename S, class TensorType>
void Serialize(S &serializer, std::shared_ptr<SaveableParamsInterface> const &nsp)
{
  OpType operation_type = nsp->op_type;
  serializer << operation_type;

  assert(operation_type != OpType::NONE);
  assert(operation_type != OpType::GRAPH);
  assert(operation_type != OpType::SUBGRAPH);

  switch (operation_type)
  {
  case OpType::PLACEHOLDER:
  {
    SerializeImplementation<S, PlaceholderSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::WEIGHTS:
  {
    SerializeImplementation<S, WeightsSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::DROPOUT:
  {
    SerializeImplementation<S, DropoutSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LEAKY_RELU:
  {
    SerializeImplementation<S, LeakyReluSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::RANDOMISED_RELU:
  {
    SerializeImplementation<S, RandomisedReluSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::SOFTMAX:
  {
    SerializeImplementation<S, SoftmaxSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::CONVOLUTION_1D:
  {
    SerializeImplementation<S, Convolution1DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::MAX_POOL_1D:
  {
    SerializeImplementation<S, MaxPool1DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::MAX_POOL_2D:
  {
    SerializeImplementation<S, MaxPool2DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::TRANSPOSE:
  {
    SerializeImplementation<S, TransposeSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::RESHAPE:
  {
    SerializeImplementation<S, ReshapeSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LAYER_FULLY_CONNECTED:
  {
    SerializeImplementation<S, FullyConnectedSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LAYER_CONVOLUTION_1D:
  {
    SerializeImplementation<S, ConvolutionLayer1DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LAYER_CONVOLUTION_2D:
  {
    SerializeImplementation<S, ConvolutionLayer2DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  default:
  {
    throw std::runtime_error("Unknown type for Serialization");
  }
  }
}

template <typename S, class TensorType>
void Deserialize(S &serializer, std::shared_ptr<SaveableParamsInterface> &nsp)
{
  OpType operation_type = OpType::NONE;
  serializer >> operation_type;

  assert(operation_type != OpType::NONE);
  assert(operation_type != OpType::GRAPH);
  assert(operation_type != OpType::SUBGRAPH);

  switch (operation_type)
  {
  case OpType::PLACEHOLDER:
  {
    DeserializeImplementation<S, PlaceholderSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::WEIGHTS:
  {
    DeserializeImplementation<S, WeightsSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::DROPOUT:
  {
    DeserializeImplementation<S, DropoutSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LEAKY_RELU:
  {
    DeserializeImplementation<S, LeakyReluSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::RANDOMISED_RELU:
  {
    DeserializeImplementation<S, RandomisedReluSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::SOFTMAX:
  {
    DeserializeImplementation<S, SoftmaxSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::CONVOLUTION_1D:
  {
    DeserializeImplementation<S, Convolution1DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::MAX_POOL_1D:
  {
    DeserializeImplementation<S, MaxPool1DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::MAX_POOL_2D:
  {
    DeserializeImplementation<S, MaxPool2DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::TRANSPOSE:
  {
    DeserializeImplementation<S, TransposeSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::RESHAPE:
  {
    DeserializeImplementation<S, ReshapeSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LAYER_FULLY_CONNECTED:
  {
    DeserializeImplementation<S, FullyConnectedSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LAYER_CONVOLUTION_1D:
  {
    DeserializeImplementation<S, ConvolutionLayer1DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  case OpType::LAYER_CONVOLUTION_2D:
  {
    DeserializeImplementation<S, ConvolutionLayer2DSaveableParams<TensorType>>(serializer, nsp);
    break;
  }
  default:
  {
    throw std::runtime_error("Unknown type for Serialization");
  }
  }
}

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

  template <typename S>
  friend void Serialize(S &serializer, GraphSaveableParams const &gsp)
  {
    Serialize(serializer, gsp.op_type);
    serializer << gsp.connections;

    for (auto const &node : gsp.nodes)
    {
      serializer << node.first;
      Serialize<S, TensorType>(serializer, node.second);
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, GraphSaveableParams &gsp)
  {
    Deserialize(serializer, gsp.op_type);
    serializer >> gsp.connections;
    auto num_nodes = gsp.connections.size();
    for (SizeType i = 0; i < num_nodes; i++)
    {
      std::string node_name;
      serializer >> node_name;

      std::shared_ptr<SaveableParamsInterface> nsp_ptr;
      //      std::shared_ptr<SaveableParamsInterface> nsp_ptr =
      //          Deserialize<S, gsp.nodes[node_name]->op_type, TensorType>(serializer);
      //
      //      gsp.nodes.insert(std::make_pair(node_name, nsp_ptr));

      Deserialize<S, TensorType>(serializer, nsp_ptr);
      //      Serialize<S, node.second->op_type, TensorType>(serializer, node.second);
    }
  }
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

  template <typename S>
  friend void Serialize(S &serializer, SubGraphSaveableParams const &gsp)
  {
    Serialize(serializer, gsp.op_type);

    // serialize parent class first
    auto base_pointer = static_cast<GraphSaveableParams<TensorType> const *>(&gsp);
    Serialize(serializer, *base_pointer);

    serializer << gsp.input_node_names;
    serializer << gsp.output_node_name;
  }

  template <typename S>
  friend void Deserialize(S &serializer, SubGraphSaveableParams &gsp)
  {
    Deserialize(serializer, gsp.op_type);

    // deserialize parent class first
    auto base_pointer = static_cast<GraphSaveableParams<TensorType> *>(&gsp);
    Deserialize(serializer, *base_pointer);

    serializer >> gsp.input_node_names;
    serializer >> gsp.output_node_name;
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

  template <class S>
  friend void Deserialize(S &serializer, WeightsSaveableParams<TensorType> &sp)
  {
    Deserialize(serializer, sp.op_type);
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

}  // namespace ml
namespace serializers {

/**
 * serializer for abs saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::AbsSaveableParams<TensorType>, D>
{
  using Type       = ml::AbsSaveableParams<TensorType>;
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
struct MapSerializer<ml::AddSaveableParams<TensorType>, D>
{
  using Type       = ml::AddSaveableParams<TensorType>;
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
struct MapSerializer<ml::ConcatenateSaveableParams<TensorType>, D>
{
  using Type       = ml::ConcatenateSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;
  static uint8_t const AXIS    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
    map.Append(AXIS, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(AXIS, sp.axis);
  }
};

/**
 * serializer for add saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::Convolution1DSaveableParams<TensorType>, D>
{
  using Type       = ml::Convolution1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const STRIDE_SIZE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
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
 * serializer for add saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::Convolution2DSaveableParams<TensorType>, D>
{
  using Type       = ml::Convolution2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const STRIDE_SIZE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
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
 * serializer for add saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::CrossEntropyLossSaveableParams<TensorType>, D>
{
  using Type       = ml::CrossEntropyLossSaveableParams<TensorType>;
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
struct MapSerializer<ml::DivideSaveableParams<TensorType>, D>
{
  using Type       = ml::DivideSaveableParams<TensorType>;
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
struct MapSerializer<ml::DropoutSaveableParams<TensorType>, D>
{
  using Type       = ml::DropoutSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const RANDOM_SEED = 2;
  static uint8_t const PROBABILITY = 3;
  static uint8_t const BUFFER      = 4;
  static uint8_t const INDEX       = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);
    map.Append(RANDOM_SEED, sp.random_seed);
    map.Append(PROBABILITY, sp.probability);
    map.Append(BUFFER, sp.buffer);
    map.Append(INDEX, sp.index);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
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
struct MapSerializer<ml::EluSaveableParams<TensorType>, D>
{
  using Type       = ml::EluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;
  static uint8_t const VALUE   = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
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
struct MapSerializer<ml::EmbeddingsSaveableParams<TensorType>, D>
{
  using Type       = ml::EmbeddingsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
    map.Append(OP_CODE, sp.op_type);

    // serialize parent class first
    auto base_pointer = static_cast<ml::WeightsSaveableParams<TensorType> const *>(&sp);
    Serialize(map_constructor, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    auto base_pointer = static_cast<ml::WeightsSaveableParams<TensorType> *>(&sp);
    Deserialize(map, *base_pointer);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::ExpSaveableParams<TensorType>, D>
{
  using Type       = ml::ExpSaveableParams<TensorType>;
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
struct MapSerializer<ml::FlattenSaveableParams<TensorType>, D>
{
  using Type       = ml::FlattenSaveableParams<TensorType>;
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
struct MapSerializer<ml::ConvolutionLayer1DSaveableParams<TensorType>, D>
{
  using Type       = ml::ConvolutionLayer1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE         = 1;
  static uint8_t const KERNEL_SIZE     = 1;
  static uint8_t const INPUT_CHANNELS  = 1;
  static uint8_t const OUTPUT_CHANNELS = 1;
  static uint8_t const STRIDE_SIZE     = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OP_CODE, sp.output_channels);
    map.Append(OUTPUT_CHANNELS, sp.stride_size);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    Serialize(map_constructor, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(INPUT_CHANNELS, sp.input_channels);
    map.ExpectKeyGetValue(OP_CODE, sp.output_channels);
    map.ExpectKeyGetValue(OUTPUT_CHANNELS, sp.stride_size);

    // deserialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    Deserialize(map, *base_pointer);
  }
};

/**
 * serializer for Conv2d layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::ConvolutionLayer2DSaveableParams<TensorType>, D>
{
  using Type       = ml::ConvolutionLayer2DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE         = 1;
  static uint8_t const KERNEL_SIZE     = 1;
  static uint8_t const INPUT_CHANNELS  = 1;
  static uint8_t const OUTPUT_CHANNELS = 1;
  static uint8_t const STRIDE_SIZE     = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(5);
    map.Append(OP_CODE, sp.op_type);
    map.Append(KERNEL_SIZE, sp.kernel_size);
    map.Append(INPUT_CHANNELS, sp.input_channels);
    map.Append(OP_CODE, sp.output_channels);
    map.Append(OUTPUT_CHANNELS, sp.stride_size);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    Serialize(map_constructor, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(INPUT_CHANNELS, sp.input_channels);
    map.ExpectKeyGetValue(OP_CODE, sp.output_channels);
    map.ExpectKeyGetValue(OUTPUT_CHANNELS, sp.stride_size);

    // deserialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    Deserialize(map, *base_pointer);
  }
};

/**
 * serializer for Conv2d layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::FullyConnectedSaveableParams<TensorType>, D>
{
  using Type       = ml::FullyConnectedSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE  = 1;
  static uint8_t const IN_SIZE  = 2;
  static uint8_t const OUT_SIZE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);
    map.Append(OP_CODE, sp.op_type);
    map.Append(IN_SIZE, sp.in_size);
    map.Append(OUT_SIZE, sp.out_size);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    Serialize(map_constructor, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.in_size);
    map.ExpectKeyGetValue(OUT_SIZE, sp.out_size);

    // deserialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    Deserialize(map, *base_pointer);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::LeakyReluSaveableParams<TensorType>, D>
{
  using Type       = ml::LeakyReluSaveableParams<TensorType>;
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
struct MapSerializer<ml::LeakyReluOpSaveableParams<TensorType>, D>
{
  using Type       = ml::LeakyReluOpSaveableParams<TensorType>;
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
struct MapSerializer<ml::LogSaveableParams<TensorType>, D>
{
  using Type       = ml::LogSaveableParams<TensorType>;
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
struct MapSerializer<ml::LogSigmoidSaveableParams<TensorType>, D>
{
  using Type       = ml::LogSigmoidSaveableParams<TensorType>;
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
struct MapSerializer<ml::LogSoftmaxSaveableParams<TensorType>, D>
{
  using Type       = ml::LogSoftmaxSaveableParams<TensorType>;
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
struct MapSerializer<ml::MatrixMultiplySaveableParams<TensorType>, D>
{
  using Type       = ml::MatrixMultiplySaveableParams<TensorType>;
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
struct MapSerializer<ml::MaxPool1DSaveableParams<TensorType>, D>
{
  using Type       = ml::MaxPool1DSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE     = 1;
  static uint8_t const KERNEL_SIZE = 2;
  static uint8_t const STRIDE_SIZE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
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
struct MapSerializer<ml::MaxPool2DSaveableParams<TensorType>, D>
{
  using Type       = ml::MaxPool2DSaveableParams<TensorType>;
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
struct MapSerializer<ml::MeanSquareErrorSaveableParams<TensorType>, D>
{
  using Type       = ml::MeanSquareErrorSaveableParams<TensorType>;
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
struct MapSerializer<ml::MaximumSaveableParams<TensorType>, D>
{
  using Type       = ml::MaximumSaveableParams<TensorType>;
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
struct MapSerializer<ml::MultiplySaveableParams<TensorType>, D>
{
  using Type       = ml::MultiplySaveableParams<TensorType>;
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
struct MapSerializer<ml::PlaceholderSaveableParams<TensorType>, D>
{
  using Type       = ml::PlaceholderSaveableParams<TensorType>;
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
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::RandomisedReluSaveableParams<TensorType>, D>
{
  using Type       = ml::RandomisedReluSaveableParams<TensorType>;
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
    map.Append(LOWER_BOUND, sp.op_type);
    map.Append(UPPER_BOUND, sp.op_type);
    map.Append(RANDOM_SEED, sp.op_type);
    map.Append(BUFFER, sp.op_type);
    map.Append(INDEX, sp.op_type);
    map.Append(RANDOM_VALUE, sp.op_type);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(LOWER_BOUND, sp.op_type);
    map.ExpectKeyGetValue(UPPER_BOUND, sp.op_type);
    map.ExpectKeyGetValue(RANDOM_SEED, sp.op_type);
    map.ExpectKeyGetValue(BUFFER, sp.op_type);
    map.ExpectKeyGetValue(INDEX, sp.op_type);
    map.ExpectKeyGetValue(RANDOM_VALUE, sp.op_type);
  }
};

/**
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::ReluSaveableParams<TensorType>, D>
{
  using Type       = ml::ReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(7);
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
struct MapSerializer<ml::ReshapeSaveableParams<TensorType>, D>
{
  using Type       = ml::ReshapeSaveableParams<TensorType>;
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
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::SigmoidSaveableParams<TensorType>, D>
{
  using Type       = ml::SigmoidSaveableParams<TensorType>;
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
struct MapSerializer<ml::SoftmaxSaveableParams<TensorType>, D>
{
  using Type       = ml::SoftmaxSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;
  static uint8_t const AXIS    = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
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
struct MapSerializer<ml::SoftmaxCrossEntropySaveableParams<TensorType>, D>
{
  using Type       = ml::SoftmaxCrossEntropySaveableParams<TensorType>;
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
struct MapSerializer<ml::SQRTSaveableParams<TensorType>, D>
{
  using Type       = ml::SQRTSaveableParams<TensorType>;
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
struct MapSerializer<ml::SubtractSaveableParams<TensorType>, D>
{
  using Type       = ml::SubtractSaveableParams<TensorType>;
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
struct MapSerializer<ml::TanhSaveableParams<TensorType>, D>
{
  using Type       = ml::TanhSaveableParams<TensorType>;
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
struct MapSerializer<ml::TransposeSaveableParams<TensorType>, D>
{
  using Type       = ml::TransposeSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE          = 1;
  static uint8_t const TRANSPOSE_VECTOR = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);
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
 * serializer for Embeddings saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::WeightsSaveableParams<TensorType>, D>
{
  using Type       = ml::WeightsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE               = 1;
  static uint8_t const OUTPUT_PRESENT        = 2;
  static uint8_t const OUTPUT                = 3;
  static uint8_t const REGULARISER           = 4;
  static uint8_t const REGULARISATION_RATE   = 5;
  static uint8_t const GRADIENT_ACCUMULATION = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(6);
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
    map.Append(REGULARISER, static_cast<int>(sp.regularisation_type));
    map.Append(REGULARISATION_RATE, sp.regularisation_type);
    map.Append(GRADIENT_ACCUMULATION, sp.gradient_accumulation);
  }
};

};  // namespace serializers
}  // namespace fetch
