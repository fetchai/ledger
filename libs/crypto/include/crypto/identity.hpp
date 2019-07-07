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
#include "crypto/signature_register.hpp"

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
  Identity(uint8_t identity_parameters, byte_array::ConstByteArray identifier)
    : identity_parameters_{std::move(identity_parameters)}
    , identifier_{std::move(identifier)}
  {}

  Identity(byte_array::ConstByteArray identifier)
    : identifier_{std::move(identifier)}
  {}

  uint8_t const &parameters() const
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

  void SetParameters(uint8_t p)
  {
    identity_parameters_ = std::move(p);
  }

  operator bool() const
  {
    return TestIdentityParameterSize(identity_parameters_, identifier_.size());
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
    identifier_ = identifier_.Copy();
  }

private:
  uint8_t                    identity_parameters_{edcsa_curve_type::sn};
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
  uint8_t               params;
  byte_array::ByteArray id;
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
  std::size_t operator()(fetch::crypto::Identity const &value) const noexcept
  {
    fetch::crypto::FNV hashStream;
    hashStream.Update(value.identifier());

    auto const params = value.parameters();
    hashStream.Update(&params, sizeof(decltype(params)));

    auto const        res = hashStream.Final();
    std::size_t const out = *reinterpret_cast<std::size_t const *const>(res.pointer());

    return out;
  }
};

}  // namespace std
