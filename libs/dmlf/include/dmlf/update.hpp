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
  using TensorType       = T;
  using VectorTensorType = std::vector<TensorType>;
  using TimeStampType    = UpdateInterface::TimeStampType;
  using FingerprintType  = UpdateInterface::FingerprintType;

  using PayloadType = VectorTensorType;

  explicit Update()
    : stamp_{CurrentTime()}
  {}
  explicit Update(VectorTensorType gradients)
    : stamp_{CurrentTime()}
    , gradients_{std::move(gradients)}
    , fingerprint_{ComputeFingerprint()}
  {}

  ~Update() override = default;

  byte_array::ByteArray Serialise() override
  {
    serializers::MsgPackSerializer serializer;
    serializer << *this;
    return serializer.data();
  }
  byte_array::ByteArray Serialise(std::string type) override
  {
    serializers::MsgPackSerializer serializer;
    serializers::MsgPackSerializer serializer_;
    serializer_ << *this;
    serializer << type;
    serializer << serializer_.data();
    return serializer.data();
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
  FingerprintType Fingerprint() const override
  {
    return fingerprint_;
  }

  virtual const VectorTensorType &GetGradients() const
  {
    return gradients_;
  }

  Update(const Update &other) = delete;
  Update &operator=(const Update &other)  = delete;
  bool    operator==(const Update &other) = delete;
  bool    operator<(const Update &other)  = delete;

protected:
private:
  static TimeStampType CurrentTime()
  {
    return static_cast<TimeStampType>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count());
  }

  FingerprintType ComputeFingerprint()
  {
    serializers::MsgPackSerializer serializer;
    serializer << gradients_;
    return crypto::Hash<crypto::SHA256>(serializer.data());
  }

  TimeStampType    stamp_;
  VectorTensorType gradients_;
  FingerprintType  fingerprint_;
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

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &update)
  {
    auto map = map_constructor(3);
    map.Append(TIME_STAMP, update.stamp_);
    map.Append(GRADIENTS, update.gradients_);
    map.Append(FINGERPRINT, update.fingerprint_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &update)
  {
    map.ExpectKeyGetValue(TIME_STAMP, update.stamp_);
    map.ExpectKeyGetValue(GRADIENTS, update.gradients_);
    map.ExpectKeyGetValue(FINGERPRINT, update.fingerprint_);
  }
};

}  // namespace serializers
}  // namespace fetch
