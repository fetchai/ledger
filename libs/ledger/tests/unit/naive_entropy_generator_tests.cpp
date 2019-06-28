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

#include "random_address.hpp"

#include "core/random/lcg.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "ledger/consensus/naive_entropy_generator.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace {

using fetch::crypto::SHA256;
using fetch::crypto::Hash;
using fetch::random::LinearCongruentialGenerator;
using fetch::ledger::NaiveEntropyGenerator;
using fetch::ledger::Digest;
using fetch::ledger::EntropyGeneratorInterface;

using RNG                      = LinearCongruentialGenerator;
using NaiveEntropyGeneratorPtr = std::unique_ptr<NaiveEntropyGenerator>;

class NaiveEntropyGeneratorTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rng_.Seed(58);
    naive_entropy_generator_ = std::make_unique<NaiveEntropyGenerator>();
  }

  void TearDown() override
  {
    naive_entropy_generator_.reset();
  }

  Digest GenerateRandomDigest()
  {
    auto address = GenerateRandomAddress(rng_);
    return address.address();  // take advantage that these are the same length
  }

  uint64_t CalculateEntropy(Digest reference)
  {
    for (std::size_t i = 0; i < NaiveEntropyGenerator::ROUNDS; ++i)
    {
      reference = Hash<SHA256>(reference);
    }

    auto const &digest_ref = reference;
    auto const *entropy    = reinterpret_cast<uint64_t const *>(digest_ref.pointer());
    return *entropy;
  }

  RNG                      rng_;
  NaiveEntropyGeneratorPtr naive_entropy_generator_;
};

TEST_F(NaiveEntropyGeneratorTests, SimpleCheck)
{
  Digest reference_digest = GenerateRandomDigest();

  uint64_t const expected_entropy = CalculateEntropy(reference_digest);
  uint64_t       actual_entropy   = 0;

  EXPECT_EQ(EntropyGeneratorInterface::Status::OK,
            naive_entropy_generator_->GenerateEntropy(reference_digest, 0, actual_entropy));
  EXPECT_EQ(expected_entropy, actual_entropy);
}

}  // namespace
