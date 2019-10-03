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

#include "dmlf/update.hpp"

namespace fetch {
namespace dmlf {
  
  Update::Update()
    : stamp_{CurrentTime()}
  {}
  Update::Update(VectorTensorType gradients)
    : stamp_{CurrentTime()}
    , gradients_{std::move(gradients)}
    , fingerprint_{ComputeFingerprint()}
  {}

  byte_array::ByteArray Update::serialise() override
  {
    serializers::MsgPackSerializer serializer;
    serializer << *this;
    return serializer.data();
  }
  byte_array::ByteArray Update::serialise(std::string type) override
  {
    serializers::MsgPackSerializer serializer;
    serializers::MsgPackSerializer serializer_;
    serializer_ << *this;
    serializer << type;
    serializer << serializer_.data();
    return serializer.data();
  }
  void Update::deserialise(const byte_array::ByteArray &map) override
  {
    serializers::MsgPackSerializer serializer{map};
    serializer >> *this;
  }
  TimeStampType Update::TimeStamp() const override
  {
    return stamp_;
  }
  FingerprintType Update::Fingerprint() const override
  {
    return fingerprint_;
  }

  virtual const VectorTensorType &GetGradients() const
  {
    return gradients_;
  }

  TimeStampType Update::CurrentTime()
  {
    return static_cast<TimeStampType>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count());
  }

  FingerprintType Update::ComputeFingerprint()
  {
    serializers::MsgPackSerializer serializer;
    serializer << gradients_;
    return crypto::Hash<crypto::SHA256>(serializer.data());
  }


}  // namespace dmlf
}  // namespace fetch
