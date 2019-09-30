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
#include "core/random/lcg.hpp"
#include "crypto/mcl_dkg.hpp"

#include "benchmark/benchmark.h"

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::random::LinearCongruentialGenerator;

namespace {
using RNG = LinearCongruentialGenerator;
RNG rng;

template <std::size_t LENGTH>
ConstByteArray GenerateRandomData()
{
  static constexpr std::size_t RNG_WORD_SIZE = sizeof(RNG::RandomType);
  static constexpr std::size_t NUM_WORDS     = LENGTH / RNG_WORD_SIZE;

  static_assert((LENGTH % RNG_WORD_SIZE) == 0, "Size must be a multiple of random type");

  ByteArray buffer;
  buffer.Resize(LENGTH);

  auto *words = reinterpret_cast<RNG::RandomType *>(buffer.pointer());
  for (std::size_t i = 0; i < NUM_WORDS; ++i)
  {
    *words++ = rng();
  }

  return ConstByteArray{buffer};
}

void SignBLSSignature(benchmark::State &state)
{
  bn::initPairing();
  // Generate a random message
  ConstByteArray msg = GenerateRandomData<256>();

  // Create keys
  auto     committee_size = static_cast<uint32_t>(state.range(0));
  uint32_t threshold      = committee_size / 2 + 1;
  auto     outputs = fetch::crypto::mcl::TrustedDealerGenerateKeys(committee_size, threshold);
  assert(outputs.size() == committee_size);

  // Randomly select index to sign
  auto index = static_cast<uint32_t>(rng() % committee_size);

  for (auto _ : state)
  {
    // Sign message
    fetch::crypto::mcl::SignShare(msg, outputs[index].secret_key_share);
  }
}

void VerifyBLSSignature(benchmark::State &state)
{
  bn::initPairing();
  // Generate a random message
  ConstByteArray msg = GenerateRandomData<256>();

  // Create keys
  auto     committee_size = static_cast<uint32_t>(state.range(0));
  uint32_t threshold      = committee_size / 2 + 1;
  auto     outputs = fetch::crypto::mcl::TrustedDealerGenerateKeys(committee_size, threshold);

  // Randomly select index to sign
  auto sign_index = static_cast<uint32_t>(rng() % committee_size);
  auto signature  = fetch::crypto::mcl::SignShare(msg, outputs[sign_index].secret_key_share);

  // Randomly select another index to verify
  auto verify_index = static_cast<uint32_t>(rng() % committee_size);
  bool check;
  for (auto _ : state)
  {
    // Verify message
    check = fetch::crypto::mcl::VerifySign(outputs[verify_index].public_key_shares[sign_index], msg,
                                           signature);
  }
  assert(check);
}

void ComputeGroupSignature(benchmark::State &state)
{
  bn::initPairing();
  // Generate a random message
  ConstByteArray msg = GenerateRandomData<256>();

  // Create keys
  auto     committee_size = static_cast<uint32_t>(state.range(0));
  uint32_t threshold      = committee_size / 2 + 1;
  auto     outputs = fetch::crypto::mcl::TrustedDealerGenerateKeys(committee_size, threshold);

  // Randomly select indices to sign
  std::unordered_map<uint32_t, fetch::crypto::mcl::Signature> threshold_signatures;
  while (threshold_signatures.size() < threshold)
  {
    auto sign_index = static_cast<uint32_t>(rng() % committee_size);
    auto signature  = fetch::crypto::mcl::SignShare(msg, outputs[sign_index].secret_key_share);
    threshold_signatures.insert({sign_index, signature});
  }

  fetch::crypto::mcl::Signature group_sig;
  for (auto _ : state)
  {
    // Compute group signature
    group_sig = fetch::crypto::mcl::LagrangeInterpolation(threshold_signatures);
  }
  auto index = static_cast<uint32_t>(rng() % committee_size);
  assert(fetch::crypto::mcl::VerifySign(outputs[index].group_public_key, msg, group_sig));
}
}  // namespace

BENCHMARK(SignBLSSignature)->Range(50, 200);
BENCHMARK(VerifyBLSSignature)->Range(50, 200);
BENCHMARK(ComputeGroupSignature)->Range(50, 200);
