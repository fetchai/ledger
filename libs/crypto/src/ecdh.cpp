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

#include "crypto/ecdsa.hpp"
#include "crypto/ecdh.hpp"

namespace fetch {
namespace crypto {

/**
 * Compute the shared key using Ellipic Curve Diffe-Hellman between two parties
 *
 * @param signer The reference to the signer object
 * @param verifier The reference to the verifier object
 * @param shared_key The generated shared key
 * @return true if successful, otherwise false
 */
bool ComputeSharedKey(ECDSASigner const &signer, ECDSAVerifier const &verifier,
                      byte_array::ConstByteArray &shared_key)
{
  (void)signer;
  (void)verifier;
  (void)shared_key;

  return false;
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
  bool succes{false};

  auto const *ecdsa_signer = dynamic_cast<ECDSASigner const *>(&prover);
  auto const *ecdsa_verifier = dynamic_cast<ECDSAVerifier const *>(&verifier);

  if ((ecdsa_signer != nullptr) && (ecdsa_verifier != nullptr))
  {
    succes = ComputeSharedKey(*ecdsa_signer, *ecdsa_verifier, shared_key);
  }

  return succes;
}

} // namespace crypto
} // namespace fetch