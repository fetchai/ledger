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
#include "crypto/verifier.hpp"

namespace fetch {
namespace crypto {

/**
 * Build the corresponding Verifier based from the provided identity
 *
 * @param identity
 * @return
 */
std::unique_ptr<Verifier> Verifier::Build(Identity const &identity)
{
  std::unique_ptr<Verifier> verifier;

  // only supported signature scheme currently
  verifier = std::make_unique<ECDSAVerifier>(identity);

  return verifier;
}

bool Verifier::Verify(Identity const &identity, ConstByteArray const &data,
                      ConstByteArray const &signature)
{
  // build a compatible verifier
  auto verifier = Build(identity);

  // determine if the signature is valid
  return verifier->Verify(data, signature);
}

/**
 * This function, given a key, a buffer and a signature, verifies authenticity of the three.
 *
 * @param identity The public key to construct a concrete (e.g. ECDSA) verifier from
 * @param data Message to be verified
 * @param signature
 */
bool Verify(byte_array::ConstByteArray key, byte_array::ConstByteArray const &data,
            byte_array::ConstByteArray const &signature)
{
  return ECDSAVerifier(Identity(std::move(key))).Verify(data, signature);
}

}  // namespace crypto
}  // namespace fetch
