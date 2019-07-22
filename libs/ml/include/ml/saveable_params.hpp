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

#include "math/base_types.hpp"
#include "ml/ops/ops.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "core/serializers/byte_array_buffer.hpp"

#include <typeinfo>

namespace fetch {
namespace ml {

/* Generic container for all the saveable params of an op.
 * Some ops will have to declare subclasses (substructs?) of this.
 */
struct SaveableParams
{
  using SelfType = SaveableParams;
  static constexpr char const * sp_descriptor = "SaveableParams";  // description of saveparams type
  virtual ~SaveableParams() = default;
  std::string DESCRIPTOR;  // description of op

  virtual void Serialize(fetch::serializers::ByteArrayBuffer & serializer){
    serializer << sp_descriptor;
  }

  template <class S>
  void Deserialize(S & serializer){
    FETCH_UNUSED(serializer);
  }
};

template <class ArrayType>
struct WeightsSaveableParams : public SaveableParams
{
  static constexpr char const * sp_descriptor = "WeightsSaveableParams";  // description of saveparams type
  std::shared_ptr<ArrayType>             output{};
  fetch::ml::details::RegularisationType regularisation_type{};
  typename ArrayType::Type               regularisation_rate;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << *output;
    serializer << static_cast<int>(regularisation_type);
    serializer << regularisation_rate;
  }

  template <class S>
  void Deserialize(S & serializer) {
    ArrayType output_temp{};
    serializer >> output_temp;
    output = std::make_shared<ArrayType>(output_temp);
    int reg_type_int{};
    serializer >> reg_type_int;
    regularisation_type = static_cast<fetch::ml::details::RegularisationType>(reg_type_int);
    serializer >> regularisation_rate;
  }
};

template <class ArrayType>
struct DropoutSaveableParams : public SaveableParams
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;
  static constexpr char const * sp_descriptor = "DropoutSaveableParams";
  SizeType random_seed{};
  DataType probability;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << random_seed;
    serializer << probability;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> random_seed;
    serializer >> probability;
  }
};

template <class ArrayType>
struct LeakyReluSaveableParams : public SaveableParams
{
  using DataType = typename ArrayType::Type;
  static constexpr char const * sp_descriptor = "LeakyReluSaveableParams";
  DataType a;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << a;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> a;
  }
};

template <class ArrayType>
struct RandomizedReluSaveableParams : public SaveableParams
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;
  static constexpr char const * sp_descriptor = "RandomizedReluSaveableParams";
  DataType lower_bound;
  DataType upper_bound;
  SizeType random_seed;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << lower_bound << upper_bound;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> lower_bound;
    serializer >> upper_bound;
  }
};

struct SoftmaxSaveableParams : public SaveableParams
{
  static constexpr char const * sp_descriptor = "SoftmaxSaveableParams";
  fetch::math::SizeType axis;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << axis;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> axis;
  }
};

struct Convolution1DSaveableParams : public SaveableParams
{
  static constexpr char const * sp_descriptor = "Convolution1DSaveableParams";
  fetch::math::SizeType stride_size;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << stride_size;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> stride_size;
  }
};

struct MaxPoolSaveableParams : public SaveableParams
{
  static constexpr char const * sp_descriptor = "MaxPoolSaveableParams";
  fetch::math::SizeType kernel_size;
  fetch::math::SizeType stride_size;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << kernel_size;
    serializer << stride_size;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> kernel_size;
    serializer >> stride_size;
  }
};

struct TransposeSaveableParams : public SaveableParams
{
  static constexpr char const * sp_descriptor = "TransposeSaveableParams";
  std::vector<fetch::math::SizeType> transpose_vector;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << transpose_vector;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> transpose_vector;
  }
};

struct ReshapeSaveableParams : public SaveableParams
{
  static constexpr char const * sp_descriptor = "ReshapeSaveableParams";
  std::vector<fetch::math::SizeType> new_shape;

  void Serialize(fetch::serializers::ByteArrayBuffer & serializer) override {
    serializer << sp_descriptor;
    serializer << new_shape;
  }

  template <class S>
  void Deserialize(S & serializer) {
    serializer >> new_shape;
  }
};

template <class S, class SP>
std::shared_ptr<SaveableParams> DeserializeHelper(S  &serializer)
{
  auto nsp_ptr = std::make_shared<SP>();
  nsp_ptr->Deserialize(serializer);
  return nsp_ptr;
}

template <class ArrayType>
struct GraphSaveableParams
{
  using DataType = typename ArrayType::Type;
  using SizeType = typename ArrayType::SizeType;
  // unique node name to list of inputs
  std::vector<std::pair<std::string, std::vector<std::string>>>    connections;
  std::map<std::string, std::shared_ptr<SaveableParams>> nodes;

  template <typename S>
  friend void Serialize(S &serializer, GraphSaveableParams const &gsp)
  {
    serializer << gsp.connections;
    for (auto const & node: gsp.nodes){
      serializer << node.first;
      node.second->Serialize(serializer);
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, GraphSaveableParams &gsp)
  {
    serializer >> gsp.connections;
    auto num_nodes = gsp.connections.size();
    for (SizeType i=0; i<num_nodes; i++){
      std::string node_name;
      serializer >> node_name;

      std::string next_sp_descriptor;
      serializer >> next_sp_descriptor;
//      std::cout << "next_sp_descriptor: " << next_sp_descriptor << std::endl;

      std::shared_ptr<SaveableParams> nsp_ptr;
      if (next_sp_descriptor == SaveableParams::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, SaveableParams>(serializer);
      } else if (next_sp_descriptor == WeightsSaveableParams<ArrayType>::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, WeightsSaveableParams<ArrayType>>
        (serializer);
      } else if (next_sp_descriptor == DropoutSaveableParams<ArrayType>::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, DropoutSaveableParams<ArrayType>>
        (serializer);
      } else if (next_sp_descriptor == LeakyReluSaveableParams<ArrayType>::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, LeakyReluSaveableParams<ArrayType>>
        (serializer);
      } else if (next_sp_descriptor == RandomizedReluSaveableParams<ArrayType>::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, RandomizedReluSaveableParams<ArrayType>>
        (serializer);
      } else if (next_sp_descriptor == SoftmaxSaveableParams::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, SoftmaxSaveableParams>(serializer);
      } else if (next_sp_descriptor == Convolution1DSaveableParams::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, Convolution1DSaveableParams>(serializer);
      } else if (next_sp_descriptor == MaxPoolSaveableParams::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, MaxPoolSaveableParams>(serializer);
      } else if (next_sp_descriptor == TransposeSaveableParams::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, TransposeSaveableParams>(serializer);
      } else if (next_sp_descriptor == ReshapeSaveableParams::sp_descriptor) {
        nsp_ptr = DeserializeHelper<S, ReshapeSaveableParams>(serializer);
      } else {
        throw std::runtime_error("Unknown type for deserialization");
      }

      gsp.nodes.insert(std::make_pair(node_name, nsp_ptr));
    }
  }
};

}  // namespace ml
}  // namespace fetch
