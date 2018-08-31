#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/byte_array/byte_array.hpp"
namespace fetch {
namespace crypto {

class Identity
{
public:
  Identity() {}

  Identity(byte_array::ConstByteArray identity_paramters, byte_array::ConstByteArray identifier)
    : identity_paramters_(identity_paramters.Copy()), identifier_(identifier.Copy())
  {}

  Identity(Identity const &other)
    : identity_paramters_(other.identity_paramters_), identifier_(other.identifier_)
  {}

  byte_array::ConstByteArray const &parameters() const { return identity_paramters_; }

  byte_array::ConstByteArray const &identifier() const { return identifier_; }

  void SetIdentifier(byte_array::ConstByteArray const &ident) { identifier_ = ident.Copy(); }

  void SetParameters(byte_array::ConstByteArray const &params)
  {
    identity_paramters_ = params.Copy();
  }

  bool is_valid() const { return is_valid_; }

  operator bool() { return is_valid(); }

  bool operator<(Identity const &other) const {
    return identifier_ < other.identifier_;
  }

  static Identity CreateInvalid()
  {
    Identity id;
    id.is_valid_ = false;
    return id;
  }

private:
  bool                       is_valid_ = true;
  byte_array::ConstByteArray identity_paramters_;
  byte_array::ConstByteArray identifier_;
};

static inline Identity InvalidIdentity() { return Identity::CreateInvalid(); }

template <typename T>
T &Serialize(T &serializer, Identity const &data)
{
  serializer << data.is_valid();
  serializer << data.parameters();
  serializer << data.identifier();

  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, Identity &data)
{
  byte_array::ByteArray params, id;
  bool                  valid;
  serializer >> valid;
  serializer >> params;
  serializer >> id;
  if (valid)
  {
    data.SetParameters(params);
    data.SetIdentifier(id);
  }
  else
  {
    data = InvalidIdentity();
  }

  return serializer;
}

}  // namespace crypto
}  // namespace fetch
