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
#include "core/random/lcg.hpp"
#include "crypto/mcl_dkg.hpp"

#include "benchmark/benchmark.h"

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::crypto::mcl::PublicKey;
using fetch::crypto::mcl::Generator;
using fetch::crypto::mcl::Signature;
using fetch::crypto::mcl::AggregatePrivateKey;
using fetch::crypto::mcl::AggregatePublicKey;
using RNG = fetch::random::LinearCongruentialGenerator;

namespace {

RNG rng;

ConstByteArray GenerateRandomData(std::size_t length)
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

void SignatureAggregationCoefficient(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  Generator generator;
  fetch::crypto::mcl::SetGenerator(generator);

  // Create keys
  auto                   cabinet_size = static_cast<uint32_t>(state.range(0));
  std::vector<PublicKey> notarisation_public_keys;

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    auto keys = fetch::crypto::mcl::GenerateKeyPair(generator);
    notarisation_public_keys.push_back(keys.second);
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    auto sign_index = static_cast<uint32_t>(rng() % cabinet_size);
    state.ResumeTiming();

    // Compute aggregate signature coefficient
    fetch::crypto::mcl::SignatureAggregationCoefficient(notarisation_public_keys[sign_index],
                                                        notarisation_public_keys);
  }
}

void AggregateSign(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  Generator generator;
  fetch::crypto::mcl::SetGenerator(generator);

  // Create keys
  auto                             cabinet_size = static_cast<uint32_t>(state.range(0));
  std::vector<AggregatePrivateKey> aggregate_private_keys;
  std::vector<PublicKey>           notarisation_public_keys;

  aggregate_private_keys.resize(cabinet_size);

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    auto keys                             = fetch::crypto::mcl::GenerateKeyPair(generator);
    aggregate_private_keys[i].private_key = keys.first;
    notarisation_public_keys.push_back(keys.second);
  }

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    aggregate_private_keys[i].coefficient = fetch::crypto::mcl::SignatureAggregationCoefficient(
        notarisation_public_keys[i], notarisation_public_keys);
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    // Generate a random message
    ConstByteArray msg        = GenerateRandomData(256);
    auto           sign_index = static_cast<uint32_t>(rng() % cabinet_size);
    state.ResumeTiming();

    // Compute aggregate signature coefficient
    fetch::crypto::mcl::AggregateSign(msg, aggregate_private_keys[sign_index]);
  }
}

void VerifyAggregateSignatureOptimal(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  Generator generator;
  fetch::crypto::mcl::SetGenerator(generator);

  // Create keys
  auto                             cabinet_size = static_cast<uint32_t>(state.range(0));
  uint32_t                         threshold    = cabinet_size / 2 + 1;
  std::vector<AggregatePrivateKey> aggregate_private_keys;
  std::vector<AggregatePublicKey>  aggregate_public_keys;
  std::vector<PublicKey>           notarisation_public_keys;

  aggregate_private_keys.resize(cabinet_size);
  aggregate_public_keys.resize(cabinet_size);

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    auto keys                             = fetch::crypto::mcl::GenerateKeyPair(generator);
    aggregate_private_keys[i].private_key = keys.first;
    notarisation_public_keys.push_back(keys.second);
  }

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    aggregate_private_keys[i].coefficient = fetch::crypto::mcl::SignatureAggregationCoefficient(
        notarisation_public_keys[i], notarisation_public_keys);
    aggregate_public_keys[i] =
        AggregatePublicKey{notarisation_public_keys[i], aggregate_private_keys[i].coefficient};
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    // Generate a random message
    ConstByteArray msg = GenerateRandomData(256);
    // Randomly select indices to sign
    std::unordered_map<uint32_t, fetch::crypto::mcl::Signature> signatures;
    while (signatures.size() < threshold)
    {
      auto sign_index = static_cast<uint32_t>(rng() % cabinet_size);
      auto signature  = fetch::crypto::mcl::AggregateSign(msg, aggregate_private_keys[sign_index]);
      signatures.insert({sign_index, signature});
    }
    auto aggregate_signature =
        fetch::crypto::mcl::ComputeAggregateSignature(signatures, cabinet_size);
    state.ResumeTiming();

    // Verify aggregate signature
    auto aggregate_public_key = fetch::crypto::mcl::ComputeAggregatePublicKey(
        aggregate_signature.second, aggregate_public_keys);
    fetch::crypto::mcl::VerifySign(aggregate_public_key, msg, aggregate_signature.first, generator);
  }
}

void VerifyAggregateSignatureSlow(benchmark::State &state)
{
  fetch::crypto::mcl::details::MCLInitialiser();
  Generator generator;
  fetch::crypto::mcl::SetGenerator(generator);

  // Create keys
  auto                             cabinet_size = static_cast<uint32_t>(state.range(0));
  uint32_t                         threshold    = cabinet_size / 2 + 1;
  std::vector<AggregatePrivateKey> aggregate_private_keys;
  std::vector<PublicKey>           notarisation_public_keys;

  aggregate_private_keys.resize(cabinet_size);

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    auto keys                             = fetch::crypto::mcl::GenerateKeyPair(generator);
    aggregate_private_keys[i].private_key = keys.first;
    notarisation_public_keys.push_back(keys.second);
  }

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    aggregate_private_keys[i].coefficient = fetch::crypto::mcl::SignatureAggregationCoefficient(
        notarisation_public_keys[i], notarisation_public_keys);
  }

  for (auto _ : state)
  {
    state.PauseTiming();
    // Generate a random message
    ConstByteArray msg = GenerateRandomData(256);
    // Randomly select indices to sign
    std::unordered_map<uint32_t, fetch::crypto::mcl::Signature> signatures;
    while (signatures.size() < threshold)
    {
      auto sign_index = static_cast<uint32_t>(rng() % cabinet_size);
      auto signature  = fetch::crypto::mcl::AggregateSign(msg, aggregate_private_keys[sign_index]);
      signatures.insert({sign_index, signature});
    }
    auto aggregate_signature =
        fetch::crypto::mcl::ComputeAggregateSignature(signatures, cabinet_size);
    state.ResumeTiming();

    // Verify aggregate signature
    auto aggregate_public_key = fetch::crypto::mcl::ComputeAggregatePublicKey(
        aggregate_signature.second, notarisation_public_keys);
    fetch::crypto::mcl::VerifySign(aggregate_public_key, msg, aggregate_signature.first, generator);
  }
}

}  // namespace

BENCHMARK(SignatureAggregationCoefficient)->RangeMultiplier(2)->Range(50, 500);
BENCHMARK(AggregateSign)->RangeMultiplier(2)->Range(50, 500);
BENCHMARK(VerifyAggregateSignatureOptimal)->RangeMultiplier(2)->Range(50, 500);
BENCHMARK(VerifyAggregateSignatureSlow)->RangeMultiplier(2)->Range(50, 500);
