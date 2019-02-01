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

#include <utility>

#include "core/byte_array/byte_array.hpp"
#include "crypto/fnv.hpp"
#include "crypto/openssl_common.hpp"

namespace fetch {
namespace crypto {

class Identity
{
public:
  using edcsa_curve_type = crypto::openssl::ECDSACurve<NID_secp256k1>;

  Identity()
  {}

  Identity(Identity const &other) = default;
  Identity &operator=(Identity const &other) = default;
  Identity(Identity &&other)                 = default;
  Identity &operator=(Identity &&other) = default;

  // Fully relying on caller that it will behave = will NOT modify value passed
  // (Const)ByteArray(s)
  Identity(byte_array::ConstByteArray identity_parameters, byte_array::ConstByteArray identifier)
    : identity_parameters_{std::move(identity_parameters)}
    , identifier_{std::move(identifier)}
  {}

  Identity(byte_array::ConstByteArray identifier)
    : identifier_{std::move(identifier)}
  {}

  byte_array::ConstByteArray const &parameters() const
  {
    return identity_parameters_;
  }

  byte_array::ConstByteArray const &identifier() const
  {
    return identifier_;
  }

  void SetIdentifier(byte_array::ConstByteArray const &ident)
  {
    identifier_ = ident;
  }

  void SetParameters(byte_array::ConstByteArray const &params)
  {
    identity_parameters_ = params;
  }

  operator bool() const
  {
    return identity_parameters_ == edcsa_curve_type::sn &&
           identifier_.size() == edcsa_curve_type::publicKeySize;
  }

  static Identity CreateInvalid()
  {
    Identity id;
    return id;
  }

  bool operator==(Identity const &right) const
  {
    return identity_parameters_ == right.identity_parameters_ && identifier_ == right.identifier_;
  }

  bool operator<(Identity const &right) const
  {
    if (identifier_ < right.identifier_)
    {
      return true;
    }
    else if (identifier_ == right.identifier_)
    {
      return identity_parameters_ < right.identity_parameters_;
    }

    return false;
  }

  void Clone()
  {
    identifier_          = identifier_.Copy();
    identity_parameters_ = identity_parameters_.Copy();
  }

private:
  byte_array::ConstByteArray identity_parameters_{edcsa_curve_type::sn};
  byte_array::ConstByteArray identifier_;
};

static inline Identity InvalidIdentity()
{
  return Identity::CreateInvalid();
}

template <typename T>
T &Serialize(T &serializer, Identity const &data)
{
  serializer << data.identifier();
  serializer << data.parameters();

  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, Identity &data)
{
  byte_array::ByteArray params, id;
  serializer >> id;
  serializer >> params;

  data.SetParameters(params);
  data.SetIdentifier(id);
  if (!data)
  {
    data = InvalidIdentity();
  }

  return serializer;
}

}  // namespace crypto
}  // namespace fetch

namespace std {

template <>
struct hash<fetch::crypto::Identity>
{
  std::size_t operator()(fetch::crypto::Identity const &value) const
  {
    fetch::crypto::FNV hashStream;
    hashStream.Update(value.identifier());
    hashStream.Update(value.parameters());
    return hashStream.Final<>();
  }
};

}  // namespace std
