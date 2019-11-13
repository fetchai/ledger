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
#include "ml/dataloaders/word2vec_loaders/vocab.hpp"

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
  using VectorTensor     = std::vector<TensorType>;
  using TimeStampType    = UpdateInterface::TimeStampType;
  using Fingerprint      = UpdateInterface::Fingerprint;
  using HashType         = byte_array::ConstByteArray;
  using PubKeyB64        = std::string;
  using ReverseVocabType = std::vector<std::string>;

  using Payload = VectorTensor;

  explicit Update()
    : stamp_{CurrentTime()}
  {}
  explicit Update(VectorTensor gradients)
    : stamp_{CurrentTime()}
    , gradients_{std::move(gradients)}
    , fingerprint_{ComputeFingerprint()}
  {}

  explicit Update(VectorTensor gradients, byte_array::ConstByteArray hash, ReverseVocabType vocab)
    : stamp_{CurrentTime()}
    , gradients_{std::move(gradients)}
    , fingerprint_{ComputeFingerprint()}
    , hash_{std::move(hash)}
    , vocab_{std::move(vocab)}
  {}

  ~Update() override = default;

  byte_array::ByteArray Serialise() override
  {
    serializers::LargeObjectSerializeHelper serializer;
    serializer << *this;
    return serializer.data();
  }
  byte_array::ByteArray Serialise(std::string type) override
  {
    serializers::LargeObjectSerializeHelper serializer;
    serializers::LargeObjectSerializeHelper serializer_;
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
  Fingerprint GetFingerprint() const override
  {
    return fingerprint_;
  }

  void SetSource(PubKeyB64 public_key)
  {
    source_ = std::move(public_key);
  }

  PubKeyB64 const &GetSource()
  {
    return source_;
  }

  virtual VectorTensor const &GetGradients() const
  {
    return gradients_;
  }

  virtual HashType const &GetHash() const
  {
    return hash_;
  }

  virtual ReverseVocabType const &GetReverseVocab() const
  {
    return vocab_;
  }

  Update(Update const &other)
  {
    this->stamp_       = other.stamp_;
    this->gradients_   = other.gradients_;
    this->fingerprint_ = other.fingerprint_;
    this->hash_        = other.hash_;
    this->source_      = other.source_;
    this->vocab_       = other.vocab_;
  }

  Update &operator=(Update const &other) = delete;

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
    return crypto::Hash<crypto::SHA256>(serializer.data());
  }

  TimeStampType stamp_;
  VectorTensor  gradients_;
  Fingerprint   fingerprint_;
  PubKeyB64     source_;

  HashType         hash_;
  ReverseVocabType vocab_;
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
  static uint8_t const HASH        = 4;
  static uint8_t const SOURCE      = 5;
  static uint8_t const VOCAB       = 6;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &update)
  {
    auto map = map_constructor(6);
    map.Append(TIME_STAMP, update.stamp_);
    map.Append(GRADIENTS, update.gradients_);
    map.Append(FINGERPRINT, update.fingerprint_);
    map.Append(HASH, update.hash_);
    map.Append(SOURCE, update.source_);
    map.Append(VOCAB, update.vocab_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &update)
  {
    map.ExpectKeyGetValue(TIME_STAMP, update.stamp_);
    map.ExpectKeyGetValue(GRADIENTS, update.gradients_);
    map.ExpectKeyGetValue(FINGERPRINT, update.fingerprint_);
    map.ExpectKeyGetValue(HASH, update.hash_);
    map.ExpectKeyGetValue(SOURCE, update.source_);
    map.ExpectKeyGetValue(VOCAB, update.vocab_);
  }
};

}  // namespace serializers
}  // namespace fetch
