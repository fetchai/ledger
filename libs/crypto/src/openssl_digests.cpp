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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/openssl_digests.hpp"

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <cassert>
#include <cstddef>
#include <vector>

namespace fetch {
namespace crypto {

namespace {

EVP_MD const *to_openssl_type(OpenSslDigestType const type)
{
  EVP_MD const *openssl_type = nullptr;
  switch (type)
  {
  case OpenSslDigestType::MD5:
    openssl_type = EVP_md5();
    break;
  case OpenSslDigestType::SHA1:
    openssl_type = EVP_sha1();
    break;
  case OpenSslDigestType::SHA2_512:
    openssl_type = EVP_sha512();
    break;
  }

  assert(openssl_type);

  return openssl_type;
}

std::size_t to_digest_size(OpenSslDigestType const type)
{
  std::size_t digest_size = 0u;
  switch (type)
  {
  case OpenSslDigestType::MD5:
    digest_size = MD5_DIGEST_LENGTH;
    break;
  case OpenSslDigestType::SHA1:
    digest_size = SHA_DIGEST_LENGTH;
    break;
  case OpenSslDigestType::SHA2_512:
    digest_size = SHA512_DIGEST_LENGTH;
    break;
  }

  assert(digest_size);

  return digest_size;
}

}  // namespace

namespace internal {

class OpenSslDigestImpl
{
public:
  explicit OpenSslDigestImpl(OpenSslDigestType type)
    : digest_size_bytes_{to_digest_size(type)}
    , openssl_type_{to_openssl_type(type)}
    , ctx_{EVP_MD_CTX_create()}
  {}

  std::size_t const   digest_size_bytes_;
  EVP_MD const *const openssl_type_;
  EVP_MD_CTX *const   ctx_;
};

}  // namespace internal

OpenSslDigestContext::OpenSslDigestContext(OpenSslDigestType type)
  : impl_{new internal::OpenSslDigestImpl(type)}
{}

OpenSslDigestContext::~OpenSslDigestContext()
{
  delete impl_;
}

void OpenSslDigestContext::reset() const
{
  EVP_DigestInit_ex(impl_->ctx_, impl_->openssl_type_, nullptr);
}

fetch::byte_array::ConstByteArray OpenSslDigestContext::operator()(
    fetch::byte_array::ConstByteArray const &input) const
{
  fetch::byte_array::ByteArray output(impl_->digest_size_bytes_);

  EVP_DigestUpdate(impl_->ctx_, input.pointer(), input.size());
  EVP_DigestFinal_ex(impl_->ctx_, output.pointer(), nullptr);

  return output;
}

}  // namespace crypto
}  // namespace fetch
