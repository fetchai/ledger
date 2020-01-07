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
#include "crypto/block_cipher.hpp"
#include "crypto/ecdh.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/hash.hpp"
#include "crypto/secure_channel.hpp"
#include "crypto/sha256.hpp"

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;

namespace fetch {
namespace crypto {
namespace {

const BlockCipher::Type CIPHER_TYPE          = BlockCipher::AES_256_CBC;
const std::size_t       IV_BIT_SIZE          = BlockCipher::GetIVLength(CIPHER_TYPE);
const std::size_t       IV_BYTE_SIZE         = (IV_BIT_SIZE + 7u) / 8u;
const std::size_t       HASH_SIZE            = 32;
const std::size_t       MINIMUM_PAYLOAD_SIZE = HASH_SIZE + 1;

ConstByteArray GenerateIV(uint16_t service, uint16_t channel, uint16_t counter)
{
  // implementation assumes 16 byte iv - i.e. AES
  assert(IV_BYTE_SIZE == 16);

  ByteArray iv{};
  iv.Resize(IV_BYTE_SIZE);

  *reinterpret_cast<uint16_t *>(&iv[1])  = service;
  *reinterpret_cast<uint16_t *>(&iv[6])  = channel;
  *reinterpret_cast<uint16_t *>(&iv[11]) = counter;

  return {iv};
}

ECDSAVerifier GenerateVerifier(ConstByteArray const &public_key)
{
  return ECDSAVerifier{Identity{SECP256K1_UNCOMPRESSED, public_key}};
}

}  // namespace

SecureChannel::SecureChannel(Prover const &prover)
  : prover_{prover}
{}

bool SecureChannel::Encrypt(ConstByteArray const &destination_public_key, uint16_t service,
                            uint16_t channel, uint16_t counter, ConstByteArray const &payload,
                            ConstByteArray &encrypted_payload)
{
  bool success{false};

  ECDSAVerifier verifier{GenerateVerifier(destination_public_key)};

  // attempt to compute the shared key
  ConstByteArray shared_key{};
  if (ComputeSharedKey(prover_, verifier, shared_key))
  {
    ConstByteArray iv = GenerateIV(service, channel, counter);

    // generate the complete payload by concatenating the payload with the hash of the payload
    auto const payload_digest = Hash<SHA256>(payload);

    ByteArray generated_payload{};
    generated_payload.Append(payload, payload_digest);

    // encrypt the payload
    success = BlockCipher::Encrypt(BlockCipher::AES_256_CBC, shared_key, iv, generated_payload,
                                   encrypted_payload);
  }

  return success;
}

bool SecureChannel::Decrypt(ConstByteArray const &sender_public_key, uint16_t service,
                            uint16_t channel, uint16_t counter,
                            ConstByteArray const &encrypted_payload, ConstByteArray &payload)
{
  bool success{false};

  ECDSAVerifier verifier{GenerateVerifier(sender_public_key)};

  // attempt to compute the shared key
  ConstByteArray shared_key{};
  if (ComputeSharedKey(prover_, verifier, shared_key))
  {
    ConstByteArray iv = GenerateIV(service, channel, counter);

    // encrypt the payload
    ConstByteArray decrypted_payload{};
    bool const decryption_success = BlockCipher::Decrypt(BlockCipher::AES_256_CBC, shared_key, iv,
                                                         encrypted_payload, decrypted_payload);

    // ensure the decryption was a success and that we have the required payload size
    if (decryption_success && (decrypted_payload.size() >= MINIMUM_PAYLOAD_SIZE))
    {
      // split the payload from the hash
      ConstByteArray split_payload =
          decrypted_payload.SubArray(0, decrypted_payload.size() - HASH_SIZE);
      ConstByteArray const split_digest =
          decrypted_payload.SubArray(decrypted_payload.size() - HASH_SIZE, HASH_SIZE);

      // compute the expected digest
      ConstByteArray const expected_digest = Hash<SHA256>(split_payload);

      if (expected_digest == split_digest)
      {
        payload = std::move(split_payload);
        success = true;
      }
    }
  }

  return success;
}

}  // namespace crypto
}  // namespace fetch
