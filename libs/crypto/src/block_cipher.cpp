//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "aes.hpp"

#include "crypto/block_cipher.hpp"

namespace fetch {
namespace crypto {
namespace {

bool IsAES(BlockCipher::Type type) noexcept
{
  bool is_aes{false};

  switch (type)
  {
  case BlockCipher::AES_256_CBC:
    is_aes = true;
    break;
  }

  return is_aes;
}

}  // namespace

std::size_t BlockCipher::GetKeyLength(Type type) noexcept
{
  std::size_t key_length{0};

  if (IsAES(type))
  {
    key_length = AesBlockCipher::GetKeyLength(type);
  }

  return key_length;
}

std::size_t BlockCipher::GetIVLength(Type type) noexcept
{
  std::size_t iv_length{0};

  if (IsAES(type))
  {
    iv_length = AesBlockCipher::GetIVLength(type);
  }

  return iv_length;
}

bool BlockCipher::Encrypt(Type type, ConstByteArray const &key, ConstByteArray const &iv,
                          ConstByteArray const &clear_text, ConstByteArray &cipher_text)
{
  bool success{false};

  if (IsAES(type))
  {
    success = AesBlockCipher::Encrypt(type, key, iv, clear_text, cipher_text);
  }

  return success;
}

bool BlockCipher::Decrypt(Type type, ConstByteArray const &key, ConstByteArray const &iv,
                          ConstByteArray const &cipher_text, ConstByteArray &clear_text)
{
  bool success{false};

  if (IsAES(type))
  {
    success = AesBlockCipher::Decrypt(type, key, iv, cipher_text, clear_text);
  }

  return success;
}

}  // namespace crypto
}  // namespace fetch
