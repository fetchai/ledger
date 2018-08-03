#pragma once
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
  ProofOfWork(header_type header) { header_ = header; }

  bool operator()()
  {
    crypto::SHA256 hasher;
    hasher.Reset();
    hasher.Update(header_);
    hasher.Update(*this);
    hasher.Final();
    digest_ = hasher.digest();
    hasher.Reset();
    hasher.Update(digest_);
    hasher.Final();

    digest_ = hasher.digest();

    return digest_ < target_;
  }

  void SetTarget(std::size_t zeros)
  {
    target_ = 1;
    target_ <<= 8 * sizeof(uint8_t) * super_type::size() - 1 - zeros;
  }

  void SetHeader(byte_array::ByteArray header)
  {
    header_ = header;
    assert(header_ == header);
  }

  header_type const &header() const { return header_; }
  math::BigUnsigned  digest() const { return digest_; }
  math::BigUnsigned  target() const { return target_; }

private:
  math::BigUnsigned digest_;
  math::BigUnsigned target_;
  header_type       header_;
};
}  // namespace consensus
}  // namespace chain
}  // namespace fetch
