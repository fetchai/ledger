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

#include "crypto/ecdh.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace crypto {

/**
 * Compute the shared key using Elliptic Curve Diffe-Hellman between two parties
 *
 * @param signer The reference to the signer object
 * @param verifier The reference to the verifier object
 * @param shared_key The generated shared key
 * @return true if successful, otherwise false
 */
bool ComputeSharedKey(ECDSASigner const &signer, ECDSAVerifier const &verifier,
                      ConstByteArray &shared_key)
{
  bool success{false};

  auto public_ec_point = verifier.public_key().KeyAsECPoint();
  auto private_ec_key  = signer.private_key_ec_key();

  // validate the openssl pointers
  if (public_ec_point && private_ec_key)
  {
    auto const field_size = EC_GROUP_get_degree(EC_KEY_get0_group(private_ec_key.get()));

    // correctly derive the field size
    if (field_size > 0)
    {
      // compute the shared key size
      std::size_t const shared_key_size = (static_cast<std::size_t>(field_size) + 7u) / 8u;

      // create the key buffer for the shared key
      ByteArray shared_key_buffer{};
      shared_key_buffer.Resize(shared_key_size);

      // compute the shared key
      ECDH_compute_key(shared_key_buffer.pointer(), shared_key_buffer.size(), public_ec_point.get(),
                       private_ec_key.get(), nullptr);

      // return the hashed computed shared key
      shared_key = Hash<SHA256>(shared_key_buffer);
      success    = true;
    }
  }

  return success;
}

/**
 * Helper function to perform necessary dynamic casts to get call the shared compute task
 *
 * @param prover The reference to the prover
 * @param verifier The reference to the verifier
 * @param shared_key The reference to the output shared key
 * @return true if successful, otherwise false
 */
bool ComputeSharedKey(Prover const &prover, Verifier const &verifier,
                      byte_array::ConstByteArray &shared_key)
{
  bool success{false};

  auto const *ecdsa_signer   = dynamic_cast<ECDSASigner const *>(&prover);
  auto const *ecdsa_verifier = dynamic_cast<ECDSAVerifier const *>(&verifier);

  if ((ecdsa_signer != nullptr) && (ecdsa_verifier != nullptr))
  {
    success = ComputeSharedKey(*ecdsa_signer, *ecdsa_verifier, shared_key);
  }

  return success;
}

}  // namespace crypto
}  // namespace fetch
