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

#include "crypto/hasher_interface.hpp"

#include <cstddef>
#include <cstdint>

namespace fetch {

namespace byte_array {
class ConstByteArray;
}

namespace crypto {
namespace internal {

enum class OpenSslDigestType
{
  MD5,
  SHA1,
  SHA2_256,
  SHA2_512
};

class OpenSslDigestContext;

class OpenSslHasherImpl
{
public:
  OpenSslHasherImpl()                          = delete;
  OpenSslHasherImpl(OpenSslHasherImpl const &) = delete;
  OpenSslHasherImpl(OpenSslHasherImpl &&)      = delete;

  OpenSslHasherImpl &operator=(OpenSslHasherImpl const &) = delete;
  OpenSslHasherImpl &operator=(OpenSslHasherImpl &&) = delete;

private:
  explicit OpenSslHasherImpl(OpenSslDigestType);
  ~OpenSslHasherImpl();

  bool reset();
  bool update(uint8_t const *data_to_hash, std::size_t size);
  bool final(uint8_t *data_to_hash);

  OpenSslDigestContext *const ctx_;

  template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
  friend class OpenSslHasher;
};

template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
class OpenSslHasher
  : public HasherInterface<typename internal::OpenSslHasher<hash_size, EnumClass, hasher_type>>
{
public:
  using BaseType =
      HasherInterface<typename internal::OpenSslHasher<hash_size, EnumClass, hasher_type>>;

  using BaseType::Final;
  using BaseType::Reset;
  using BaseType::Update;

  static constexpr std::size_t size_in_bytes = hash_size;

  OpenSslHasher();
  ~OpenSslHasher();
  OpenSslHasher(OpenSslHasher const &) = delete;
  OpenSslHasher(OpenSslHasher &&)      = delete;

  OpenSslHasher &operator=(OpenSslHasher const &) = delete;
  OpenSslHasher &operator=(OpenSslHasher &&) = delete;

private:
  bool ResetHasherInternal();
  bool UpdateHasherInternal(uint8_t const *data_to_hash, std::size_t size);
  bool FinaliseHasherInternal(uint8_t *hash);

  internal::OpenSslHasherImpl impl_{hasher_type};

  friend BaseType;
};

template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
OpenSslHasher<hash_size, EnumClass, hasher_type>::OpenSslHasher() = default;

template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
OpenSslHasher<hash_size, EnumClass, hasher_type>::~OpenSslHasher() = default;

template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
bool OpenSslHasher<hash_size, EnumClass, hasher_type>::ResetHasherInternal()
{
  return impl_.reset();
}

template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
bool OpenSslHasher<hash_size, EnumClass, hasher_type>::UpdateHasherInternal(
    uint8_t const *data_to_hash, std::size_t const size)
{
  return impl_.update(data_to_hash, size);
}

template <std::size_t hash_size, typename EnumClass, EnumClass hasher_type>
bool OpenSslHasher<hash_size, EnumClass, hasher_type>::FinaliseHasherInternal(uint8_t *hash)
{
  return impl_.final(hash);
}

}  // namespace internal
}  // namespace crypto
}  // namespace fetch
