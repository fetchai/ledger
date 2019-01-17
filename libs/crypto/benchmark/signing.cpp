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
#include "core/random/lcg.hpp"
#include "crypto/ecdsa.hpp"

#include <benchmark/benchmark.h>
#include <stdexcept>

using fetch::crypto::ECDSASigner;
using fetch::crypto::ECDSAVerifier;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::random::LinearCongruentialGenerator;

namespace {

using RNG = LinearCongruentialGenerator;

RNG rng;

template <std::size_t LENGTH>
ConstByteArray GenerateRandomData()
{
  static constexpr std::size_t RNG_WORD_SIZE = sizeof(RNG::random_type);
  static constexpr std::size_t NUM_WORDS     = LENGTH / RNG_WORD_SIZE;

  static_assert((LENGTH % RNG_WORD_SIZE) == 0, "Size must be a multiple of random type");

  ByteArray buffer;
  buffer.Resize(LENGTH);

  RNG::random_type *words = reinterpret_cast<RNG::random_type *>(buffer.pointer());
  for (std::size_t i = 0; i < NUM_WORDS; ++i)
  {
    *words++ = rng();
  }

  return ConstByteArray{buffer};
}

void VerifySignature(benchmark::State &state)
{
  // generate a random message
  ConstByteArray msg = GenerateRandomData<2048>();

  // create the signer and verifier
  ECDSASigner   signer;
  ECDSAVerifier verifier(signer.identity());

  // create the signed data
  if (!signer.Sign(msg))
  {
    throw std::runtime_error("Unable to sign the message");
  }

  // extract the signature
  ConstByteArray signature = signer.signature();

  for (auto _ : state)
  {
    // run the verification
    verifier.Verify(msg, signature);
  }
}

}  // namespace

BENCHMARK(VerifySignature);