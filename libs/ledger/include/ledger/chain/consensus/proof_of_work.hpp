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

#include <core/byte_array/const_byte_array.hpp>
#include <crypto/sha256.hpp>
#include <math/bignumber.hpp>
namespace fetch {
namespace chain {
namespace consensus {

class ProofOfWork : public math::BigUnsigned
{
public:
  using super_type  = math::BigUnsigned;
  using header_type = byte_array::ConstByteArray;

  ProofOfWork() = default;
  ProofOfWork(header_type header)
  {
    header_ = header;
  }

  bool operator()()
  {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update(header_);
    hasher.Update(*this);
    digest_ = hasher.Final();
    hasher.Reset();
    hasher.Update(digest_);
    digest_ = hasher.Final();

    return digest_ < target_;
  }

  void SetTarget(std::size_t zeros)
  {
    target_ = 1;
    target_ <<= 8 * sizeof(uint8_t) * super_type::size() - 1 - zeros;
  }

  void SetTarget(math::BigUnsigned target)
  {
    target_ = target;
  }

  void SetHeader(byte_array::ByteArray header)
  {
    header_ = header;
    assert(header_ == header);
  }

  header_type const &header() const
  {
    return header_;
  }
  math::BigUnsigned digest() const
  {
    return digest_;
  }
  math::BigUnsigned target() const
  {
    return target_;
  }

private:
  math::BigUnsigned digest_;
  math::BigUnsigned target_;
  header_type       header_;
};

template <typename T>
inline void Serialize(T &serializer, ProofOfWork const &p)
{
  serializer << p.header() << p.target();
}

template <typename T>
inline void Deserialize(T &serializer, ProofOfWork &p)
{
  ProofOfWork::header_type header;
  math::BigUnsigned        target, digest;
  serializer >> header >> target;
  p.SetHeader(header);
  p.SetTarget(target);
}

}  // namespace consensus
}  // namespace chain
}  // namespace fetch
