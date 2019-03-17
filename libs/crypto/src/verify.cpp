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
 * A "factory" function. Since all it does is,
 * instantiates an implementation of an interface with but a single practical method
 * (plus one getter), it seamlessly applies this method internally.
 *
 * This function, given a key, a buffer and a signature,
 * in some magic way verifies authenticity of the three.
 *
 * It can be done by applying all known concrete verifiers in sequence, until first one confirms.
 *
 * (This is actually what is done now, and the fact that there's but a single concrete
 * implementation to the Verifier interface is definitely beneficial.)
 *
 * Or, the concrete algorithm could be guessed based on the bit pattern, simply by a watermark put
 * in by the prover.
 *
 * Whatever, this function encapsulates the instantiation of a concrete verifier and the
 * verification itself. It (possibly) turns three byte arrays into a single bit.
 *
 * @param identity The public key to construct a concrete (likely, ECDSA) verifier from
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
