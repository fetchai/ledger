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
                             std::shared_ptr<fetch::ml::SaveableParamsInterface> const &op)
{
  auto castnode = std::dynamic_pointer_cast<SP>(op);
  map.Append(code, *castnode);
}

template <class TensorType, typename D, class SP, typename MapType>
void DeserializeImplementation(MapType &map, uint8_t code,
                               std::shared_ptr<fetch::ml::SaveableParamsInterface> const &op)
{
  SP sp_interface;
  map.ExpectKeyGetValue(code, sp_interface);
  op = std::dynamic_pointer_cast<fetch::ml::SaveableParamsInterface>(sp_interface);
}
}  // namespace

template <class TensorType, typename D, typename MapType>
void SerializeAnyOp(MapType &map, uint8_t code, fetch::ml::OpType const &op_type,
                    std::shared_ptr<fetch::ml::SaveableParamsInterface> const &op)
{
  switch (op_type)
  {
  case ml::OpType::PLACEHOLDER:
  {
    SerializeImplementation<TensorType, D, ml::PlaceholderSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::WEIGHTS:
  {
    SerializeImplementation<TensorType, D, ml::WeightsSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::DROPOUT:
  {
    SerializeImplementation<TensorType, D, ml::DropoutSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::LEAKY_RELU:
  {
    SerializeImplementation<TensorType, D, ml::LeakyReluSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::RANDOMISED_RELU:
  {
    SerializeImplementation<TensorType, D, ml::RandomisedReluSaveableParams<TensorType>>(map, code,
                                                                                         op);
    break;
  }
  case ml::OpType::SOFTMAX:
  {
    SerializeImplementation<TensorType, D, ml::SoftmaxSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::CONVOLUTION_1D:
  {
    SerializeImplementation<TensorType, D, ml::Convolution1DSaveableParams<TensorType>>(map, code,
                                                                                        op);
    break;
  }
  case ml::OpType::MAX_POOL_1D:
  {
    SerializeImplementation<TensorType, D, ml::MaxPool1DSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::MAX_POOL_2D:
  {
    SerializeImplementation<TensorType, D, ml::MaxPool2DSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::TRANSPOSE:
  {
    SerializeImplementation<TensorType, D, ml::TransposeSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::RESHAPE:
  {
    SerializeImplementation<TensorType, D, ml::ReshapeSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::LAYER_FULLY_CONNECTED:
  {
    SerializeImplementation<TensorType, D, ml::FullyConnectedSaveableParams<TensorType>>(map, code,
                                                                                         op);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_1D:
  {
    SerializeImplementation<TensorType, D, ml::ConvolutionLayer1DSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_2D:
  {
    SerializeImplementation<TensorType, D, ml::ConvolutionLayer2DSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  default:
  {
    throw std::runtime_error("Unknown type for Serialization");
  }
  }
}

template <class TensorType, typename D>
void DeserializeAnyOp(interfaces::MapDeserializer<D> &map, uint8_t code,
                      fetch::ml::OpType const &                            op_type,
                      std::shared_ptr<fetch::ml::SaveableParamsInterface> &op)
{

  switch (op_type)
  {
  case ml::OpType::PLACEHOLDER:
  {
    DeserializeImplementation<TensorType, D, ml::PlaceholderSaveableParams<TensorType>>(map, code,
                                                                                        op);
    break;
  }
  case ml::OpType::WEIGHTS:
  {
    DeserializeImplementation<TensorType, D, ml::WeightsSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::DROPOUT:
  {
    DeserializeImplementation<TensorType, D, ml::DropoutSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::LEAKY_RELU:
  {
    DeserializeImplementation<TensorType, D, ml::LeakyReluSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::RANDOMISED_RELU:
  {
    DeserializeImplementation<TensorType, D, ml::RandomisedReluSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::SOFTMAX:
  {
    DeserializeImplementation<TensorType, D, ml::SoftmaxSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::CONVOLUTION_1D:
  {
    DeserializeImplementation<TensorType, D, ml::Convolution1DSaveableParams<TensorType>>(map, code,
                                                                                          op);
    break;
  }
  case ml::OpType::MAX_POOL_1D:
  {
    DeserializeImplementation<TensorType, D, ml::MaxPool1DSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::MAX_POOL_2D:
  {
    DeserializeImplementation<TensorType, D, ml::MaxPool2DSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::TRANSPOSE:
  {
    DeserializeImplementation<TensorType, D, ml::TransposeSaveableParams<TensorType>>(map, code,
                                                                                      op);
    break;
  }
  case ml::OpType::RESHAPE:
  {
    DeserializeImplementation<TensorType, D, ml::ReshapeSaveableParams<TensorType>>(map, code, op);
    break;
  }
  case ml::OpType::LAYER_FULLY_CONNECTED:
  {
    DeserializeImplementation<TensorType, D, ml::FullyConnectedSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_1D:
  {
    DeserializeImplementation<TensorType, D, ml::ConvolutionLayer1DSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  case ml::OpType::LAYER_CONVOLUTION_2D:
  {
    DeserializeImplementation<TensorType, D, ml::ConvolutionLayer2DSaveableParams<TensorType>>(
        map, code, op);
    break;
  }
  default:
  {
    throw std::runtime_error("Unknown type for Serialization");
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
    map.Append(OP_CODE, sp.op_type);

    SerializeAnyOp<TensorType, D>(map, OP, sp.op_type, sp.op);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(NAME, sp.name);
    map.ExpectKeyGetValue(CACHED_OUTPUT, sp.cached_output);
    map.ExpectKeyGetValue(CACHED_OUTPUT_STATUS, sp.cached_output_status);
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    DeserializeAnyOp<TensorType, D>(map, OP, sp.op_type, sp.op);
  }
};

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
    auto map = map_constructor(5);
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
struct MapSerializer<ml::EmbeddingsSaveableParams<TensorType>, D>
{
  using Type       = ml::EmbeddingsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE    = 1;
  static uint8_t const BASE_CLASS = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(2);
    map.Append(OP_CODE, sp.op_type);

    // serialize parent class
    auto base_pointer = static_cast<ml::WeightsSaveableParams<TensorType> const *>(&sp);
    map.Append(BASE_CLASS, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);

    auto base_pointer = static_cast<ml::WeightsSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_CLASS, *base_pointer);
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
    map.Append(OP_CODE, sp.output_channels);
    map.Append(OUTPUT_CHANNELS, sp.stride_size);
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
    map.ExpectKeyGetValue(OP_CODE, sp.output_channels);
    map.ExpectKeyGetValue(OUTPUT_CHANNELS, sp.stride_size);
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
    map.Append(OP_CODE, sp.output_channels);
    map.Append(OUTPUT_CHANNELS, sp.stride_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    // deserialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(KERNEL_SIZE, sp.kernel_size);
    map.ExpectKeyGetValue(INPUT_CHANNELS, sp.input_channels);
    map.ExpectKeyGetValue(OP_CODE, sp.output_channels);
    map.ExpectKeyGetValue(OUTPUT_CHANNELS, sp.stride_size);
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

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const IN_SIZE   = 3;
  static uint8_t const OUT_SIZE  = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(IN_SIZE, sp.in_size);
    map.Append(OUT_SIZE, sp.out_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.in_size);
    map.ExpectKeyGetValue(OUT_SIZE, sp.out_size);
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
 * serializer for MaximumSaveableParams saveable params
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
 * serializer for MultiplySaveableParams saveable params
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
 * serializer for PlaceholderSaveableParams saveable params
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
 * serializer for Conv2d layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::PReluSaveableParams<TensorType>, D>
{
  using Type       = ml::PReluSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const IN_SIZE   = 3;
  static uint8_t const OUT_SIZE  = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(IN_SIZE, sp.in_size);
    map.Append(OUT_SIZE, sp.out_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.in_size);
    map.ExpectKeyGetValue(OUT_SIZE, sp.out_size);
  }
};

/**
 * serializer for RandomisedReluSaveableParams
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
 * serializer for ReluSaveableParams
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
 * serializer for ReshapeSaveableParams
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
 * serializer for self attention layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::SelfAttentionSaveableParams<TensorType>, D>
{
  using Type       = ml::SelfAttentionSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const SUB_GRAPH = 1;
  static uint8_t const OP_CODE   = 2;
  static uint8_t const IN_SIZE   = 3;
  static uint8_t const OUT_SIZE  = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(4);

    // serialize parent class first
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> const *>(&sp);
    map.Append(SUB_GRAPH, *base_pointer);

    map.Append(OP_CODE, sp.op_type);
    map.Append(IN_SIZE, sp.in_size);
    map.Append(OUT_SIZE, sp.out_size);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::SubGraphSaveableParams<TensorType> *>(&sp);
    map.ExpectKeyGetValue(SUB_GRAPH, *base_pointer);

    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    map.ExpectKeyGetValue(IN_SIZE, sp.in_size);
    map.ExpectKeyGetValue(OUT_SIZE, sp.out_size);
  }
};

/**
 * serializer for SigmoidSaveableParams
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
 * serializer for self attention layer saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::SkipGramSaveableParams<TensorType>, D>
{
  using Type       = ml::SkipGramSaveableParams<TensorType>;
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
 * serializer for SoftmaxSaveableParams
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
 * serializer for SoftmaxCrossEntropySaveableParams
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
 * serializer for SQRTSaveableParams
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
 * serializer for SubtractSaveableParams
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
 * serializer for TanhSaveableParams
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
 * serializer for TransposeSaveableParams
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
 * serializer for WeightsSaveableParams
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::WeightsSaveableParams<TensorType>, D>
{
  using Type       = ml::WeightsSaveableParams<TensorType>;
  using DriverType = D;

  static uint8_t const OP_CODE        = 1;
  static uint8_t const OUTPUT_PRESENT = 2;
  static uint8_t const OUTPUT         = 3;

  static uint8_t const HAS_REGULARISATION  = 4;
  static uint8_t const REGULARISER         = 5;
  static uint8_t const REGULARISATION_RATE = 6;

  static uint8_t const HAS_GRADIENT          = 7;
  static uint8_t const GRADIENT_ACCUMULATION = 8;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(7);
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

    // first set the regulariser type
    bool has_regularisation;
    if (sp.regulariser)
    {
      has_regularisation = true;
      map.Append(HAS_REGULARISATION, has_regularisation);
      map.Append(REGULARISER, *(sp.regulariser));
      map.Append(REGULARISATION_RATE, sp.regularisation_rate);
    }
    else
    {
      has_regularisation = false;
      map.Append(HAS_REGULARISATION, has_regularisation);
    }

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
    map.ExpectKeyGetValue(OP_CODE, sp.op_type);
    bool output_present;
    map.ExpectKeyGetValue(OUTPUT_PRESENT, output_present);
    if (output_present)
    {
      TensorType output;
      map.ExpectKeyGetValue(OUTPUT, output);
      sp.output = std::make_shared<TensorType>(output);
    }

    bool has_regularisation;
    if (sp.regulariser)
    {
      has_regularisation = true;
      map.ExpectKeyGetValue(HAS_REGULARISATION, has_regularisation);
      map.ExpectKeyGetValue(REGULARISER, *(sp.regulariser));
      map.ExpectKeyGetValue(REGULARISATION_RATE, sp.regularisation_rate);
    }
    else
    {
      has_regularisation = false;
      map.ExpectKeyGetValue(HAS_REGULARISATION, has_regularisation);
    }

    //
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

}  // namespace serializers
}  // namespace fetch
