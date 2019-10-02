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
#include "core/macros.hpp"
#include "core/random/lcg.hpp"
#include "crypto/mcl_dkg.hpp"

#include "benchmark/benchmark.h"

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::random::LinearCongruentialGenerator;

namespace {
using RNG = LinearCongruentialGenerator;
RNG rng;

ConstByteArray GenerateRandomData(size_t length)
{
  std::size_t rng_word_size = sizeof(RNG::RandomType);
  std::size_t num_words     = length / rng_word_size;

  assert((length % rng_word_size) == 0);

  ByteArray buffer;
  buffer.Resize(length);

  auto *words = reinterpret_cast<RNG::RandomType *>(buffer.pointer());
  for (std::size_t i = 0; i < num_words; ++i)
  {
    *words++ = rng();
  }

  return ConstByteArray{buffer};
}

void SignBLSSignature(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  // Create keys
  auto     committee_size = static_cast<uint32_t>(state.range(0));
  uint32_t threshold      = committee_size / 2 + 1;
  auto     outputs = fetch::crypto::mcl::TrustedDealerGenerateKeys(committee_size, threshold);
  assert(outputs.size() == committee_size);

  // Randomly select index to sign
  auto index = static_cast<uint32_t>(rng() % committee_size);

  for (auto _ : state)
  {
    // Generate message
    state.PauseTiming();
    ConstByteArray msg = GenerateRandomData(256);
    state.ResumeTiming();
    // Sign message
    fetch::crypto::mcl::SignShare(msg, outputs[index].private_key_share);
  }
}

void VerifyBLSSignature(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  bn::G2 generator;
  fetch::crypto::mcl::SetGenerator(generator);

  // Create keys
  auto     committee_size = static_cast<uint32_t>(state.range(0));
  uint32_t threshold      = committee_size / 2 + 1;
  auto     outputs = fetch::crypto::mcl::TrustedDealerGenerateKeys(committee_size, threshold);

  // Randomly select index to sign
  auto sign_index = static_cast<uint32_t>(rng() % committee_size);
  // Randomly select another index to verify
  auto verify_index = static_cast<uint32_t>(rng() % committee_size);

  for (auto _ : state)
  {
    state.PauseTiming();
    // Generate a random message
    ConstByteArray msg = GenerateRandomData(256);
    auto signature     = fetch::crypto::mcl::SignShare(msg, outputs[sign_index].private_key_share);
    state.ResumeTiming();

    // Verify message
    fetch::crypto::mcl::VerifySign(outputs[verify_index].public_key_shares[sign_index], msg,
                                   signature, generator);
  }
}

void ComputeGroupSignature(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();

  // Create keys
  auto     committee_size = static_cast<uint32_t>(state.range(0));
  uint32_t threshold      = committee_size / 2 + 1;
  auto     outputs = fetch::crypto::mcl::TrustedDealerGenerateKeys(committee_size, threshold);

  for (auto _ : state)
  {
    state.PauseTiming();
    // Generate a random message
    ConstByteArray msg = GenerateRandomData(256);
    // Randomly select indices to sign
    std::unordered_map<uint32_t, fetch::crypto::mcl::Signature> threshold_signatures;
    while (threshold_signatures.size() < threshold)
    {
      auto sign_index = static_cast<uint32_t>(rng() % committee_size);
      auto signature  = fetch::crypto::mcl::SignShare(msg, outputs[sign_index].private_key_share);
      threshold_signatures.insert({sign_index, signature});
    }
    state.ResumeTiming();

    // Compute group signature
    fetch::crypto::mcl::LagrangeInterpolation(threshold_signatures);
  }
}
}  // namespace

BENCHMARK(SignBLSSignature)->RangeMultiplier(2)->Range(50, 500);
BENCHMARK(VerifyBLSSignature)->RangeMultiplier(2)->Range(50, 500);
BENCHMARK(ComputeGroupSignature)->RangeMultiplier(2)->Range(50, 500);
