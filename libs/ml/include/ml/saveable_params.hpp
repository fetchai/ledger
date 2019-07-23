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
#include "math/base_types.hpp"
#include "ml/ops/ops.hpp"
#include "ml/regularisers/regularisation.hpp"

#include <typeinfo>

namespace fetch {
namespace ml {

/* Generic container for all the saveable params of an op.
 * Some ops will have to declare subclasses (substructs?) of this.
 */
struct SaveableParams
{
  static constexpr char const *sp_descriptor = "SaveableParams";  // description of saveparams type
  virtual ~SaveableParams()                  = default;
  std::string DESCRIPTOR;  // description of op

  template <class S>
  friend void Serialize(S &serializer, SaveableParams const & sp)
  {
    serializer << sp.DESCRIPTOR;
  }

  template <class S>
  friend void Deserialize(S &serializer, SaveableParams & sp)
  {
    serializer >> sp.DESCRIPTOR;
  }

  virtual std::string GetDescription(){
    return sp_descriptor;
  }
};

template <class ArrayType>
struct WeightsSaveableParams : public SaveableParams
{
  static constexpr char const *sp_descriptor =
      "WeightsSaveableParams";
  std::shared_ptr<ArrayType>             output;
  fetch::ml::details::RegularisationType regularisation_type{};
  typename ArrayType::Type               regularisation_rate;

  template <class S>
  friend void Serialize(S &serializer, WeightsSaveableParams<ArrayType> const & sp)
  {
    serializer << sp.DESCRIPTOR;
    if (sp.output) {
      serializer << bool{true};
      serializer << *(sp.output);
    } else {
      serializer << bool{false};
    }
    serializer << static_cast<int>(sp.regularisation_type);
    serializer << sp.regularisation_rate;
  }

  template <class S>
  friend void Deserialize(S &serializer, WeightsSaveableParams<ArrayType> & sp)
  {
    serializer >> sp.DESCRIPTOR;
    bool has_weights{};
    serializer >> has_weights;
    if (has_weights) {
      ArrayType output_temp;
      serializer >> output_temp;
      sp.output = std::make_shared<ArrayType>(output_temp);
    }
    int reg_type_int{};
    serializer >> reg_type_int;
    sp.regularisation_type = static_cast<fetch::ml::details::RegularisationType>(reg_type_int);
    serializer >> sp.regularisation_rate;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

template <class ArrayType>
struct DropoutSaveableParams : public SaveableParams
{
  using DataType                             = typename ArrayType::Type;
  using SizeType                             = typename ArrayType::SizeType;
  static constexpr char const *sp_descriptor = "DropoutSaveableParams";
  SizeType                     random_seed{};
  DataType                     probability{};

  template <class S>
  friend void Serialize(S &serializer, DropoutSaveableParams<ArrayType> const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.random_seed;
    serializer << sp.probability;
  }

  template <class S>
  friend void Deserialize(S &serializer, DropoutSaveableParams<ArrayType> & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.random_seed;
    serializer >> sp.probability;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

template <class ArrayType>
struct LeakyReluSaveableParams : public SaveableParams
{
  using DataType                             = typename ArrayType::Type;
  static constexpr char const *sp_descriptor = "LeakyReluSaveableParams";
  DataType                     a;

  template <class S>
  friend void Serialize(S &serializer, LeakyReluSaveableParams<ArrayType> const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.a;
  }

  template <class S>
  friend void Deserialize(S &serializer, LeakyReluSaveableParams<ArrayType> & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.a;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

template <class ArrayType>
struct RandomizedReluSaveableParams : public SaveableParams
{
  using DataType                             = typename ArrayType::Type;
  using SizeType                             = typename ArrayType::SizeType;
  static constexpr char const *sp_descriptor = "RandomizedReluSaveableParams";
  DataType                     lower_bound;
  DataType                     upper_bound;
  SizeType                     random_seed;

  template <class S>
  friend void Serialize(S &serializer, RandomizedReluSaveableParams<ArrayType> const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.lower_bound << sp.upper_bound;
  }

  template <class S>
  friend void Deserialize(S &serializer, RandomizedReluSaveableParams<ArrayType> & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.lower_bound;
    serializer >> sp.upper_bound;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

struct SoftmaxSaveableParams : public SaveableParams
{
  static constexpr char const *sp_descriptor = "SoftmaxSaveableParams";
  fetch::math::SizeType        axis;

  template <class S>
  friend void Serialize(S &serializer, SoftmaxSaveableParams const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.axis;
  }

  template <class S>
  friend void Deserialize(S &serializer, SoftmaxSaveableParams & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.axis;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

struct Convolution1DSaveableParams : public SaveableParams
{
  static constexpr char const *sp_descriptor = "Convolution1DSaveableParams";
  fetch::math::SizeType        stride_size;

  template <class S>
  friend void Serialize(S &serializer, Convolution1DSaveableParams const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.stride_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, Convolution1DSaveableParams & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.stride_size;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

struct MaxPoolSaveableParams : public SaveableParams
{
  static constexpr char const *sp_descriptor = "MaxPoolSaveableParams";
  fetch::math::SizeType        kernel_size;
  fetch::math::SizeType        stride_size;

  template <class S>
  friend void Serialize(S &serializer, MaxPoolSaveableParams const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.kernel_size;
    serializer << sp.stride_size;
  }

  template <class S>
  friend void Deserialize(S &serializer, MaxPoolSaveableParams & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.kernel_size;
    serializer >> sp.stride_size;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

struct TransposeSaveableParams : public SaveableParams
{
  static constexpr char const *      sp_descriptor = "TransposeSaveableParams";
  std::vector<fetch::math::SizeType> transpose_vector;

  template <class S>
  friend void Serialize(S &serializer, TransposeSaveableParams const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.transpose_vector;
  }

  template <class S>
  friend void Deserialize(S &serializer, TransposeSaveableParams & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.transpose_vector;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

struct ReshapeSaveableParams : public SaveableParams
{
  static constexpr char const *      sp_descriptor = "ReshapeSaveableParams";
  std::vector<fetch::math::SizeType> new_shape;

  template <class S>
  friend void Serialize(S &serializer, ReshapeSaveableParams const & sp)
  {
    serializer << sp.DESCRIPTOR;
    serializer << sp.new_shape;
  }

  template <class S>
  friend void Deserialize(S &serializer, ReshapeSaveableParams & sp)
  {
    serializer >> sp.DESCRIPTOR;
    serializer >> sp.new_shape;
  }

  std::string GetDescription() override{
    return sp_descriptor;
  }
};

template <class S, class SP>
void SerializeHelper(S &serializer, std::shared_ptr<SaveableParams> const &nsp)
{
  auto castnode = std::dynamic_pointer_cast<SP>(nsp);
  serializer << *castnode;
}

template <class S, class SP>
std::shared_ptr<SaveableParams> DeserializeHelper(S &serializer)
{
  auto nsp_ptr = std::make_shared<SP>();
  serializer >> *nsp_ptr;
  return nsp_ptr;
}

template <class ArrayType>
struct GraphSaveableParams
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;
  // unique node name to list of inputs
  std::vector<std::pair<std::string, std::vector<std::string>>> connections;
  std::map<std::string, std::shared_ptr<SaveableParams>>        nodes;

  template <typename S>
  friend void Serialize(S &serializer, GraphSaveableParams const &gsp)
  {
    serializer << gsp.connections;
    for (auto const &node : gsp.nodes)
    {
      serializer << node.first;

      std::string next_sp_descriptor = node.second->GetDescription();
      serializer << next_sp_descriptor;

      if (next_sp_descriptor == SaveableParams::sp_descriptor)
      {
        SerializeHelper<S, SaveableParams>(serializer, node.second);
      }
      else if (next_sp_descriptor == WeightsSaveableParams<ArrayType>::sp_descriptor)
      {
        SerializeHelper<S, WeightsSaveableParams<ArrayType>>(serializer, node.second);
      }
      else if (next_sp_descriptor == DropoutSaveableParams<ArrayType>::sp_descriptor)
      {
        SerializeHelper<S, DropoutSaveableParams<ArrayType>>(serializer, node.second);
      }
      else if (next_sp_descriptor == LeakyReluSaveableParams<ArrayType>::sp_descriptor)
      {
        SerializeHelper<S, LeakyReluSaveableParams<ArrayType>>(serializer, node.second);
      }
      else if (next_sp_descriptor == RandomizedReluSaveableParams<ArrayType>::sp_descriptor)
      {
        SerializeHelper<S, RandomizedReluSaveableParams<ArrayType>>(serializer, node.second);
      }
      else if (next_sp_descriptor == SoftmaxSaveableParams::sp_descriptor)
      {
        SerializeHelper<S, SoftmaxSaveableParams>(serializer, node.second);
      }
      else if (next_sp_descriptor == Convolution1DSaveableParams::sp_descriptor)
      {
        SerializeHelper<S, Convolution1DSaveableParams>(serializer, node.second);
      }
      else if (next_sp_descriptor == MaxPoolSaveableParams::sp_descriptor)
      {
        SerializeHelper<S, MaxPoolSaveableParams>(serializer, node.second);
      }
      else if (next_sp_descriptor == TransposeSaveableParams::sp_descriptor)
      {
        SerializeHelper<S, TransposeSaveableParams>(serializer, node.second);
      }
      else if (next_sp_descriptor == ReshapeSaveableParams::sp_descriptor)
      {
        SerializeHelper<S, ReshapeSaveableParams>(serializer, node.second);
      }
      else
      {
        throw std::runtime_error("Unknown type for Serialization");
      }
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, GraphSaveableParams &gsp)
  {
    serializer >> gsp.connections;
    auto num_nodes = gsp.connections.size();
    for (SizeType i = 0; i < num_nodes; i++)
    {
      std::string node_name;
      serializer >> node_name;

      std::string next_sp_descriptor;
      serializer >> next_sp_descriptor;
      //      std::cout << "next_sp_descriptor: " << next_sp_descriptor << std::endl;

      std::shared_ptr<SaveableParams> nsp_ptr;
      if (next_sp_descriptor == SaveableParams::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, SaveableParams>(serializer);
      }
      else if (next_sp_descriptor == WeightsSaveableParams<ArrayType>::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, WeightsSaveableParams<ArrayType>>(serializer);
      }
      else if (next_sp_descriptor == DropoutSaveableParams<ArrayType>::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, DropoutSaveableParams<ArrayType>>(serializer);
      }
      else if (next_sp_descriptor == LeakyReluSaveableParams<ArrayType>::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, LeakyReluSaveableParams<ArrayType>>(serializer);
      }
      else if (next_sp_descriptor == RandomizedReluSaveableParams<ArrayType>::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, RandomizedReluSaveableParams<ArrayType>>(serializer);
      }
      else if (next_sp_descriptor == SoftmaxSaveableParams::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, SoftmaxSaveableParams>(serializer);
      }
      else if (next_sp_descriptor == Convolution1DSaveableParams::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, Convolution1DSaveableParams>(serializer);
      }
      else if (next_sp_descriptor == MaxPoolSaveableParams::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, MaxPoolSaveableParams>(serializer);
      }
      else if (next_sp_descriptor == TransposeSaveableParams::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, TransposeSaveableParams>(serializer);
      }
      else if (next_sp_descriptor == ReshapeSaveableParams::sp_descriptor)
      {
        nsp_ptr = DeserializeHelper<S, ReshapeSaveableParams>(serializer);
      }
      else
      {
        throw std::runtime_error("Unknown type for deserialization");
      }

      gsp.nodes.insert(std::make_pair(node_name, nsp_ptr));
    }
  }
};

}  // namespace ml
}  // namespace fetch
