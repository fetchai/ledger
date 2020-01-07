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

#include "crypto/mcl_dkg.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>
#include <ostream>

using namespace fetch::crypto::mcl;
using namespace fetch::byte_array;

TEST(MclTests, BaseMcl)
{
  details::MCLInitialiser();
  Generator generator;
  SetGenerator(generator);
  bn::G1 P(-1, 1);

  // Checking clear operation resets to 0
  {
    PublicKey Q0;
    EXPECT_TRUE(Q0.isZero());
    Signature P0;
    EXPECT_TRUE(P0.isZero());
    PrivateKey F0;
    EXPECT_TRUE(F0.isZero());
  }

  // Serialization to string
  {
    std::string s = generator.getStr();
    PublicKey   Q2;
    Q2.setStr(s);
    std::string s2 = Q2.getStr();
    EXPECT_EQ(s2, s);
    EXPECT_EQ(Q2, generator);
  }

  // Testing basic operations for types G1, G2 and Fp used in DKG
  {
    PrivateKey power{2};
    PublicKey  gen_mult;
    PublicKey  gen_add;
    mcl::bn::G2::mul(gen_mult, generator, power);
    mcl::bn::G2::add(gen_add, generator, generator);
    EXPECT_EQ(gen_mult, gen_add);

    Signature P_mul;
    Signature P_add;
    mcl::bn::G1::mul(P_mul, P, power);
    mcl::bn::G1::add(P_add, P, P);
    EXPECT_EQ(P_mul, P_add);

    PrivateKey fr_pow;
    PrivateKey fr_mul;
    mcl::bn::Fr::pow(fr_pow, power, 2);
    mcl::bn::Fr::mul(fr_mul, power, power);
    EXPECT_EQ(fr_mul, fr_pow);
  }
}

TEST(MclDkgTests, ComputeLhsRhs)
{
  details::MCLInitialiser();

  // Construct polynomial of degree 2 (threshold = 1)
  uint32_t                threshold = 1;
  std::vector<PrivateKey> vec_a;
  Init(vec_a, threshold + 1);
  std::vector<PrivateKey> vec_b;
  Init(vec_b, threshold + 1);

  // Randomly initialise the values of a but leave b uninitialised
  for (uint32_t ii = 0; ii <= threshold; ++ii)
  {
    vec_a[ii].setRand();
    vec_b[ii].setRand();
  }

  std::vector<PublicKey> coefficients;
  Init(coefficients, threshold + 1);

  Generator group_g, group_h;
  SetGenerators(group_g, group_h);

  for (uint32_t ii = 0; ii <= threshold; ++ii)
  {
    coefficients[ii] = ComputeLHS(group_g, group_h, vec_a[ii], vec_b[ii]);
  }

  // Check compute LHS with direct computation
  std::vector<PublicKey> coefficients_test;
  Init(coefficients_test, threshold + 1);
  for (uint32_t jj = 0; jj <= threshold; ++jj)
  {
    PublicKey tmpG, tmp2G;
    tmpG.clear();
    tmp2G.clear();
    bn::G2::mul(tmpG, group_g, vec_a[jj]);
    bn::G2::mul(tmp2G, group_h, vec_b[jj]);
    bn::G2::add(coefficients_test[jj], coefficients_test[jj], tmpG);
    bn::G2::add(coefficients_test[jj], coefficients_test[jj], tmp2G);
  }

  EXPECT_EQ(coefficients, coefficients_test);

  PublicKey rhs;
  uint32_t  rank = 2;
  rhs            = ComputeRHS(rank, coefficients);

  // Check compute RHS with direct computation (increment rank as not allowed to be 0)
  PublicKey rhs_test;
  rhs_test.clear();
  for (uint32_t jj = 0; jj <= threshold; ++jj)
  {
    PublicKey tmpG;
    tmpG.clear();
    bn::G2::mul(tmpG, coefficients[jj],
                static_cast<int64_t>(pow(static_cast<double>(rank + 1), static_cast<double>(jj))));
    bn::G2::add(rhs_test, rhs_test, tmpG);
  }

  EXPECT_EQ(rhs, rhs_test);
}

TEST(MclDkgTests, Interpolation)
{
  details::MCLInitialiser();

  // Construct polynomial of degree 2
  uint32_t                degree = 2;
  std::vector<PrivateKey> vec_a;
  Init(vec_a, degree + 1);

  for (uint32_t ii = 0; ii <= degree; ++ii)
  {
    vec_a[ii].setRand();
  }

  Generator group_g, group_h;
  SetGenerators(group_g, group_h);

  // Compute values of polynomial evaluated at degree + 1 points
  uint32_t           cabinet_size = 5;
  std::set<uint32_t> member_set{0, 1, 2};
  assert(member_set.size() >= degree + 1);
  std::vector<PrivateKey> points;
  std::vector<PrivateKey> values;
  std::vector<PrivateKey> values2;
  values.resize(cabinet_size);
  for (auto index : member_set)
  {
    points.emplace_back(index + 1);
    PrivateKey pow, tmpF, s_i;
    s_i = vec_a[0];
    for (uint32_t k = 1; k < vec_a.size(); k++)
    {
      bn::Fr::pow(pow, index + 1, k);    // adjust index in computation
      bn::Fr::mul(tmpF, pow, vec_a[k]);  // j^k * a_i[k]
      bn::Fr::add(s_i, s_i, tmpF);
    }

    // Compare with values from ComputeSecretShare
    PrivateKey secret_share, secret_share1;
    ComputeShares(secret_share, secret_share1, vec_a, vec_a, index);
    EXPECT_EQ(secret_share, s_i);
    EXPECT_EQ(secret_share1, s_i);

    values[index] = s_i;
    values2.emplace_back(s_i);
  }

  EXPECT_EQ(vec_a, InterpolatePolynom(points, values2));
}

TEST(MclDkgTests, Signing)
{
  details::MCLInitialiser();

  uint32_t cabinet_size = 200;
  uint32_t threshold    = 101;

  // outputs[i] is assigned to node with index i in the cabinet
  auto outputs = TrustedDealerGenerateKeys(cabinet_size, threshold);

  Generator group_g;
  SetGenerator(group_g);

  std::string                             message = "Hello";
  std::unordered_map<uint32_t, Signature> threshold_signatures;

  // Generate random selection of cabinet members
  std::set<uint32_t> members;
  while (members.size() < threshold)
  {
    members.insert(static_cast<uint32_t>(rand()) % cabinet_size);
  }

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    // Compute signature share and validate
    Signature sig = SignShare(message, outputs[i].private_key_share);
    EXPECT_TRUE(VerifySign(outputs[i].public_key_shares[i], message, sig, group_g));

    // Accumulate signature shares from some random threshold number of members
    if (members.find(i) != members.end())
    {
      threshold_signatures.insert({i, sig});
    }
  }

  // Compute group signature from combining signature shares and validate
  Signature group_signature = LagrangeInterpolation(threshold_signatures);
  EXPECT_TRUE(VerifySign(outputs[0].group_public_key, message, group_signature, group_g));
}

TEST(MclDkgTests, GenerateKeys)
{
  details::MCLInitialiser();

  Generator generator;
  SetGenerator(generator);

  auto           keys      = GenerateKeyPair(generator);
  MessagePayload message   = "hello";
  Signature      signature = SignShare(message, keys.first);
  EXPECT_TRUE(VerifySign(keys.second, message, signature, generator));
}

TEST(MclNotarisationTests, AggregateSigningVerification)
{
  details::MCLInitialiser();

  Generator generator;
  SetGenerator(generator);

  uint32_t                         cabinet_size = 4;
  std::vector<PublicKey>           public_keys;
  std::vector<AggregatePrivateKey> aggregate_private_keys;
  std::vector<AggregatePublicKey>  aggregate_public_keys;

  aggregate_private_keys.resize(cabinet_size);
  aggregate_public_keys.resize(cabinet_size);

  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    auto new_keys                         = GenerateKeyPair(generator);
    aggregate_private_keys[i].private_key = new_keys.first;
    public_keys.push_back(new_keys.second);
  }

  // Compute coefficients
  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    aggregate_private_keys[i].coefficient =
        SignatureAggregationCoefficient(public_keys[i], public_keys);
    bn::G2::mul(aggregate_public_keys[i].aggregate_public_key, public_keys[i],
                aggregate_private_keys[i].coefficient);
  }

  MessagePayload                          message = "Hello";
  std::unordered_map<uint32_t, Signature> signatures;
  for (uint32_t i = 0; i < cabinet_size; ++i)
  {
    if (i != 0u)
    {
      Signature signature = AggregateSign(message, aggregate_private_keys[i]);
      EXPECT_TRUE(
          VerifySign(aggregate_public_keys[i].aggregate_public_key, message, signature, generator));
      signatures.insert({i, signature});
    }
  }

  auto aggregate_signature = ComputeAggregateSignature(signatures, cabinet_size);
  auto aggregate_public_key =
      ComputeAggregatePublicKey(aggregate_signature.second, aggregate_public_keys);
  EXPECT_TRUE(VerifySign(aggregate_public_key, message, aggregate_signature.first, generator));
}
