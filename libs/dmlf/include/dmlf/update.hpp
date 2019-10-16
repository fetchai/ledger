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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "dmlf/update_interface.hpp"
#include "ml/distributed_learning/distributed_learning_types.hpp"

#include "ml/utilities/word2vec_utilities.hpp"

#include <chrono>
#include <cstdint>
#include <vector>

namespace fetch {
namespace dmlf {

template <typename T>
class Update : public UpdateInterface
{
  template <typename TT, typename D>
  friend struct serializers::MapSerializer;

public:
  using TensorType    = T;
  using VectorTensor  = std::vector<TensorType>;
  using TimeStampType = UpdateInterface::TimeStampType;
  using Fingerprint   = UpdateInterface::Fingerprint;
  using UpdateType    = fetch::ml::distributed_learning::UpdateType;
  using HashType      = byte_array::ConstByteArray;

  using Payload = VectorTensor;

  explicit Update()
    : stamp_{CurrentTime()}
  {}
  explicit Update(VectorTensor gradients)
    : stamp_{CurrentTime()}
    , gradients_{std::move(gradients)}
    , fingerprint_{ComputeFingerprint()}
  {}

  explicit Update(VectorTensor gradients, std::string client_id, byte_array::ConstByteArray hash,
                  UpdateType uptype = UpdateType::GRADIENTS)
    : stamp_{CurrentTime()}
    , gradients_{std::move(gradients)}
    , fingerprint_{ComputeFingerprint()}
    , client_id_{std::move(client_id)}
    , hash_{std::move(hash)}
    , update_type_(uptype)
  {}

  ~Update() override = default;

  byte_array::ByteArray Serialise() override
  {
    serializers::LargeObjectSerializeHelper serializer;
    serializer << *this;
    return serializer.buffer.data();
  }
  byte_array::ByteArray Serialise(std::string type) override
  {
    serializers::LargeObjectSerializeHelper serializer;
    serializers::LargeObjectSerializeHelper serializer_;
    serializer_ << *this;
    serializer << type;
    serializer << serializer_.buffer.data();
    return serializer.buffer.data();
  }
  void DeSerialise(const byte_array::ByteArray &map) override
  {
    serializers::MsgPackSerializer serializer{map};
    serializer >> *this;
  }
  TimeStampType TimeStamp() const override
  {
    return stamp_;
  }
  Fingerprint GetFingerprint() const override
  {
    return fingerprint_;
  }

  virtual VectorTensor const &GetGradients() const
  {
    return gradients_;
  }

  virtual HashType const &GetHash() const
  {
    return hash_;
  }

  Update(Update const &other) = delete;
  Update &operator=(Update const &other)  = delete;
  bool    operator==(Update const &other) = delete;
  bool    operator<(Update const &other)  = delete;

protected:
private:
  static TimeStampType CurrentTime()
  {
    return static_cast<TimeStampType>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count());
  }

  Fingerprint ComputeFingerprint()
  {
    serializers::LargeObjectSerializeHelper serializer;
    serializer << gradients_;
    return crypto::Hash<crypto::SHA256>(serializer.buffer.data());
  }

  TimeStampType stamp_;
  VectorTensor  gradients_;
  Fingerprint   fingerprint_;

  std::string client_id_;
  HashType    hash_;
  UpdateType  update_type_ = UpdateType::GRADIENTS;
};

}  // namespace dmlf

namespace serializers {

template <typename T, typename D>
struct MapSerializer<fetch::dmlf::Update<T>, D>
{
public:
  using Type       = fetch::dmlf::Update<T>;
  using DriverType = D;

  static uint8_t const TIME_STAMP  = 1;
  static uint8_t const GRADIENTS   = 2;
  static uint8_t const FINGERPRINT = 3;
  static uint8_t const CLIENT_ID   = 4;
  static uint8_t const HASH        = 5;
  static uint8_t const UPDATE_TYPE = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &update)
  {
    auto map = map_constructor(6);
    map.Append(TIME_STAMP, update.stamp_);
    map.Append(GRADIENTS, update.gradients_);
    map.Append(FINGERPRINT, update.fingerprint_);
    map.Append(CLIENT_ID, update.client_id_);
    map.Append(HASH, update.hash_);
    map.Append(UPDATE_TYPE, static_cast<uint8_t>(update.update_type_));
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &update)
  {
    map.ExpectKeyGetValue(TIME_STAMP, update.stamp_);
    map.ExpectKeyGetValue(GRADIENTS, update.gradients_);
    map.ExpectKeyGetValue(FINGERPRINT, update.fingerprint_);

    map.ExpectKeyGetValue(CLIENT_ID, update.client_id_);
    map.ExpectKeyGetValue(HASH, update.hash_);

    uint8_t update_type;
    map.ExpectKeyGetValue(UPDATE_TYPE, update_type);
    update.update_type_ = static_cast<fetch::ml::distributed_learning::UpdateType>(update_type);
  }
};

}  // namespace serializers
}  // namespace fetch
