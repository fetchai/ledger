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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include "logging/logging.hpp"

#include "aes.hpp"

#include "openssl/err.h"
#include "openssl/evp.h"

using fetch::byte_array::ByteArray;

namespace fetch {
namespace crypto {
namespace {

constexpr std::size_t AES_BLOCK_SIZE = 16;

/**
 * Custom deleter for the cipher context
 */
class ContextDeleter
{
public:
  void operator()(EVP_CIPHER_CTX *ctx) const
  {
    EVP_CIPHER_CTX_free(ctx);
  }
};

/**
 * Create OpenSSL Cipher context using specialsed unique_ptr type for clean up
 *
 * @return The newly created cipher context
 */
std::unique_ptr<EVP_CIPHER_CTX, ContextDeleter> CreateCipherContext()
{
  return std::unique_ptr<EVP_CIPHER_CTX, ContextDeleter>{EVP_CIPHER_CTX_new()};
}

/**
 * Validate the input buffer length
 *
 * @param buffer_length The length of the buffer in bytes
 * @param desired_length The desired length in bits
 * @return true if successful, otherwise
 */
bool ValidateBufferLength(std::size_t buffer_length, std::size_t desired_length)
{
  bool valid{false};

  if (desired_length > 0)
  {
    std::size_t const desired_byte_length = (desired_length + 7u) / 8u;
    valid                                 = desired_byte_length == buffer_length;
  }

  return valid;
}

/**
 * Lookup the internal OpenSSL cipher object for a specified mode
 *
 * @param type The cipher mode
 * @return The specified cipher object if successful, otherwise nullptr
 */
EVP_CIPHER const *LookupCipher(BlockCipher::Type type)
{
  EVP_CIPHER const *cipher{nullptr};

  switch (type)
  {
  case BlockCipher::AES_256_CBC:
    cipher = EVP_aes_256_cbc();
    break;
  }

  return cipher;
}

void LogAllErrors()
{
  for (;;)
  {
    unsigned long const error_code = ERR_get_error();

    // exit if no further errors are present
    if (error_code == 0)
    {
      break;
    }

    FETCH_LOG_DEBUG("AES", "Error: ", error_code, " => ", ERR_error_string(error_code, nullptr));
  }
}

}  // namespace

/**
 * Determine the required key bit length for a given cipher mode
 *
 * @param type The cipher mode
 * @return The key bit length
 */
std::size_t AesBlockCipher::GetKeyLength(BlockCipher::Type type) noexcept
{
  std::size_t key_length{0};

  switch (type)
  {
  case BlockCipher::AES_256_CBC:
    key_length = 256;
    break;
  }

  return key_length;
}

/**
 * Determine the required IV bit length for a given cipher mode
 *
 * @param type The cipher mode
 * @return The IV bit length
 */
std::size_t AesBlockCipher::GetIVLength(BlockCipher::Type type) noexcept
{
  std::size_t iv_length{0};
  switch (type)
  {
  case BlockCipher::AES_256_CBC:
    iv_length = 128;
    break;
  }

  return iv_length;
}

/**
 * Encrypt the given clear text using the specified cipher, key and iv
 *
 * @param type The cipher mode
 * @param key The input key
 * @param iv The input IV
 * @param clear_text The input clear text
 * @param cipher_text The output cipher text
 * @return true if successful, otherwise false
 */
bool AesBlockCipher::Encrypt(BlockCipher::Type type, ConstByteArray const &key,
                             ConstByteArray const &iv, ConstByteArray const &clear_text,
                             ConstByteArray &cipher_text)
{
  bool success{false};

  // check the key length
  if (!ValidateBufferLength(key.size(), GetKeyLength(type)))
  {
    return false;
  }

  // check the IV length
  if (!ValidateBufferLength(iv.size(), GetIVLength(type)))
  {
    return false;
  }

  // lookup the cipher implementation
  EVP_CIPHER const *cipher = LookupCipher(type);
  if (cipher == nullptr)
  {
    LogAllErrors();
    return false;
  }

  int status{0};

  // initialise the encryption context
  auto ctx = CreateCipherContext();
  if (!ctx)
  {
    LogAllErrors();
    return false;
  }

  // configure the block cipher encryption
  status = EVP_EncryptInit(ctx.get(), cipher, key.pointer(), iv.pointer());
  if (status != 1)
  {
    LogAllErrors();
    return false;
  }

  // create the cipher text buffer
  ByteArray   cipher_text_buffer{};
  std::size_t populated_length{0};
  cipher_text_buffer.Resize(clear_text.size() + AES_BLOCK_SIZE);

  // run the plain text through the cipher
  auto remaining_length = static_cast<int>(cipher_text_buffer.size() - populated_length);
  status = EVP_EncryptUpdate(ctx.get(), cipher_text_buffer.pointer(), &remaining_length,
                             clear_text.pointer(), static_cast<int>(clear_text.size()));
  if (status != 1)
  {
    LogAllErrors();
    return false;
  }

  // after a successful call openssl will write back the populated length of the buffer
  populated_length += static_cast<std::size_t>(remaining_length);

  // update the remaining length and offset pointer
  uint8_t *remaining_buffer = cipher_text_buffer.pointer() + populated_length;
  remaining_length          = static_cast<int>(cipher_text_buffer.size() - populated_length);

  status = EVP_EncryptFinal_ex(ctx.get(), remaining_buffer, &remaining_length);
  if (status != 1)
  {
    LogAllErrors();
    return false;
  }

  // after a successful call openssl will write back the populated length of the buffer
  populated_length += static_cast<std::size_t>(remaining_length);

  // resize the buffer back down to the correct size
  cipher_text_buffer.Resize(populated_length);

  // update the cipher text output buffer
  cipher_text = std::move(cipher_text_buffer);
  success     = true;

  return success;
}

/**
 * Decrypt cipher text given a specified mode, key and IV
 *
 * @param type The cipher mode
 * @param key The input key
 * @param iv The input IV
 * @param cipher_text The input cipher text
 * @param clear_text The output clear text
 * @return true if successful, otherwise false
 */
bool AesBlockCipher::Decrypt(BlockCipher::Type type, ConstByteArray const &key,
                             ConstByteArray const &iv, ConstByteArray const &cipher_text,
                             ConstByteArray &clear_text)
{
  bool success{false};

  // check the key length
  if (!ValidateBufferLength(key.size(), GetKeyLength(type)))
  {
    return false;
  }

  // check the IV length
  if (!ValidateBufferLength(iv.size(), GetIVLength(type)))
  {
    return false;
  }

  // lookup the cipher implementation
  EVP_CIPHER const *cipher = LookupCipher(type);
  if (cipher == nullptr)
  {
    LogAllErrors();
    return false;
  }

  int status{0};

  // initialise the encryption context
  auto ctx = CreateCipherContext();
  if (!ctx)
  {
    LogAllErrors();
    return false;
  }

  // configure the block cipher encryption
  status =
      EVP_DecryptInit(ctx.get(), cipher, reinterpret_cast<unsigned char const *>(key.pointer()),
                      reinterpret_cast<unsigned char const *>(iv.pointer()));
  if (status != 1)
  {
    LogAllErrors();
    return false;
  }

  // create the clear text buffer at the same length of the cipher text (in practise can be one
  // block smaller)
  ByteArray clear_text_buffer{};
  clear_text_buffer.Resize(cipher_text.size() + AES_BLOCK_SIZE);
  std::size_t populated_length{0};

  // run the cipher text through the cipher
  auto remaining_length = static_cast<int>(clear_text_buffer.size() - populated_length);

  status = EVP_DecryptUpdate(ctx.get(), clear_text_buffer.pointer(), &remaining_length,
                             cipher_text.pointer(), static_cast<int>(cipher_text.size()));
  if (status != 1)
  {
    LogAllErrors();
    return false;
  }

  // after a successful call openssl will write back the populated length of the buffer
  populated_length += static_cast<std::size_t>(remaining_length);

  uint8_t *remaining_buffer = clear_text_buffer.pointer() + populated_length;
  remaining_length          = static_cast<int>(clear_text_buffer.size() - populated_length);

  status = EVP_DecryptFinal_ex(ctx.get(), remaining_buffer, &remaining_length);
  if (status != 1)
  {
    LogAllErrors();
    return false;
  }

  // after a successful call openssl will write back the populated length of the buffer
  populated_length += static_cast<std::size_t>(remaining_length);

  // resize and update the clear text buffer
  clear_text_buffer.Resize(populated_length);
  clear_text = std::move(clear_text_buffer);
  success    = true;

  return success;
}

}  // namespace crypto
}  // namespace fetch
