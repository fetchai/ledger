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

#include "crypto/openssl_hasher.hpp"

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <cassert>
#include <memory>

namespace fetch {
namespace crypto {
namespace internal {

namespace {

EVP_MD const *EvpType(OpenSslDigestType const type)
{
  switch (type)
  {
  case OpenSslDigestType::MD5:
    return EVP_md5();

  case OpenSslDigestType::SHA1:
    return EVP_sha1();

  case OpenSslDigestType::SHA2_256:
    return EVP_sha256();

  case OpenSslDigestType::SHA2_512:
    return EVP_sha512();
  }

  assert(false);
  return nullptr;
}

std::size_t DigestSize(OpenSslDigestType const type)
{
  switch (type)
  {
  case OpenSslDigestType::MD5:
    return MD5_DIGEST_LENGTH;

  case OpenSslDigestType::SHA1:
    return SHA_DIGEST_LENGTH;

  case OpenSslDigestType::SHA2_256:
    return SHA256_DIGEST_LENGTH;

  case OpenSslDigestType::SHA2_512:
    return SHA512_DIGEST_LENGTH;
  }

  assert(false);
  return 0;
}

}  // namespace

class OpenSslHasherImpl
{
public:
  explicit OpenSslHasherImpl(OpenSslDigestType type);
  ~OpenSslHasherImpl();

  std::size_t const   digest_size_bytes;
  EVP_MD const *const evp_type;
  EVP_MD_CTX *const   evp_ctx;
};

OpenSslHasherImpl::OpenSslHasherImpl(OpenSslDigestType type)
  : digest_size_bytes{DigestSize(type)}
  , evp_type{EvpType(type)}
  , evp_ctx{EVP_MD_CTX_create()}
{}

OpenSslHasherImpl::~OpenSslHasherImpl()
{
  EVP_MD_CTX_destroy(evp_ctx);
}

bool OpenSslHasher::Reset()
{
  auto const success = EVP_DigestInit_ex(impl_->evp_ctx, impl_->evp_type, nullptr) != 0;
  assert(success);

  return success;
}

bool OpenSslHasher::Update(uint8_t const *const data_to_hash, std::size_t const size)
{
  auto const success = EVP_DigestUpdate(impl_->evp_ctx, data_to_hash, size) != 0;
  assert(success);

  return success;
}

bool OpenSslHasher::Final(uint8_t *const hash)
{
  auto const success = EVP_DigestFinal_ex(impl_->evp_ctx, hash, nullptr) != 0;
  assert(success);

  return success;
}

OpenSslHasher::OpenSslHasher(OpenSslDigestType const type)
  : impl_(std::make_unique<OpenSslHasherImpl>(type))
{
  Reset();
}

OpenSslHasher::~OpenSslHasher() = default;

std::size_t OpenSslHasher::HashSize() const
{
  return impl_->digest_size_bytes;
}

}  // namespace internal
}  // namespace crypto
}  // namespace fetch
