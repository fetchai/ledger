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

#include "crypto/mcl_dkg.hpp"
#define MCLBN_FP_UNIT_SIZE 4

#include "gtest/gtest.h"

#include <cstdint>
#include <iostream>
#include <ostream>

using namespace fetch::crypto::mcl;
using namespace fetch::byte_array;

TEST(MclTests, BaseMcl)
{
  const char *aa = "12723517038133731887338407189719511622662176727675373276651903807414909099441";
  const char *ab = "4168783608814932154536427934509895782246573715297911553964171371032945126671";
  const char *ba = "13891744915211034074451795021214165905772212241412891944830863846330766296736";
  const char *bb = "7937318970632701341203597196594272556916396164729705624521405069090520231616";

  mcl::bn::initPairing();
  mcl::bn::G2 Q(mcl::bn::Fp2(aa, ab), mcl::bn::Fp2(ba, bb));
  mcl::bn::G1 P(-1, 1);

  // Comparison
  {
    mcl::bn::G2 Q0;
    Q0.clear();
    EXPECT_NE(Q0, Q);
    mcl::bn::G2 Q1;
    Q1.clear();
    EXPECT_EQ(Q1, Q0);
    mcl::bn::G1 P0;
    P0.clear();
    EXPECT_NE(P0, P);
    mcl::bn::G1 P1;
    P1.clear();
    EXPECT_EQ(P1, P0);
  }

  // Serialization to string
  {
    std::string s = Q.getStr();
    mcl::bn::G2 Q2;
    Q2.setStr(s);
    std::string s2 = Q2.getStr();
    EXPECT_EQ(s2, s);
    EXPECT_EQ(Q2, Q);
  }

  // Minimum sample
  {
    const mpz_class a = 123;
    const mpz_class b = 456;
    mcl::bn::Fp12   e1, e2;
    mcl::bn::pairing(e1, P, Q);
    mcl::bn::G2 aQ;
    mcl::bn::G1 bP;
    mcl::bn::G2::mul(aQ, Q, a);
    mcl::bn::G1::mul(bP, P, b);
    mcl::bn::pairing(e2, bP, aQ);
    mcl::bn::Fp12::pow(e1, e1, a * b);
    EXPECT_EQ(e1, e2);
  }

  // opsize
  EXPECT_EQ(sizeof(mcl::fp::Unit), 8);
}

TEST(MclDkgTests, ComputeLhsRhs)
{
  mcl::bn::initPairing();

  // Construct polynomial of degree 2 (threshold = 1)
  uint32_t            threshold = 1;
  std::vector<bn::Fr> vec_a;
  Init(vec_a, threshold + 1);
  std::vector<bn::Fr> vec_b;
  Init(vec_b, threshold + 1);

  // Randomly initialise the values of a but leave b uninitialised
  for (std::size_t ii = 0; ii <= threshold; ++ii)
  {
    vec_a[ii].setRand();
    vec_b[ii].setRand();
  }

  std::vector<bn::G2> coefficients;
  Init(coefficients, threshold + 1);

  bn::G2 group_g, group_h;
  SetGenerators(group_g, group_h);

  for (std::size_t ii = 0; ii <= threshold; ++ii)
  {
    coefficients[ii] = ComputeLHS(group_g, group_h, vec_a[ii], vec_b[ii]);
  }

  // Check compute LHS with direct computation
  std::vector<bn::G2> coefficients_test;
  Init(coefficients_test, threshold + 1);
  for (std::size_t jj = 0; jj <= threshold; ++jj)
  {
    bn::G2 tmpG, tmp2G;
    tmpG.clear();
    tmp2G.clear();
    bn::G2::mul(tmpG, group_g, vec_a[jj]);
    bn::G2::mul(tmp2G, group_h, vec_b[jj]);
    bn::G2::add(coefficients_test[jj], coefficients_test[jj], tmpG);
    bn::G2::add(coefficients_test[jj], coefficients_test[jj], tmp2G);
  }

  EXPECT_EQ(coefficients, coefficients_test);

  bn::G2   rhs;
  uint32_t rank = 2;
  rhs           = ComputeRHS(rank, coefficients);

  // Check compute RHS with direct computation (increment rank as not allowed to be 0)
  bn::G2 rhs_test;
  rhs_test.clear();
  for (std::size_t jj = 0; jj <= threshold; ++jj)
  {
    bn::G2 tmpG;
    tmpG.clear();
    bn::G2::mul(tmpG, coefficients[jj], static_cast<int64_t>(pow(rank + 1.0, 1.0 * jj)));
    bn::G2::add(rhs_test, rhs_test, tmpG);
  }

  EXPECT_EQ(rhs, rhs_test);
}

TEST(MclDkgTests, Interpolation)
{
  mcl::bn::initPairing();

  // Construct polynomial of degree 2
  uint32_t            degree = 2;
  std::vector<bn::Fr> vec_a;
  Init(vec_a, degree + 1);

  for (std::size_t ii = 0; ii <= degree; ++ii)
  {
    vec_a[ii].setRand();
  }

  bn::G2 group_g, group_h;
  SetGenerators(group_g, group_h);

  // Compute values of polynomial evaluated at degree + 1 points
  uint32_t           cabinet_size = 5;
  std::set<uint32_t> member_set{0, 1, 2};
  assert(member_set.size() >= degree + 1);
  std::vector<bn::Fr> points;
  std::vector<bn::Fr> values;
  std::vector<bn::Fr> values2;
  values.resize(cabinet_size);
  for (auto index : member_set)
  {
    points.emplace_back(index + 1);
    bn::Fr pow, tmpF, s_i;
    s_i = vec_a[0];
    for (uint32_t k = 1; k < vec_a.size(); k++)
    {
      bn::Fr::pow(pow, index + 1, k);    // adjust index in computation
      bn::Fr::mul(tmpF, pow, vec_a[k]);  // j^k * a_i[k]
      bn::Fr::add(s_i, s_i, tmpF);
    }

    // Compare with values from ComputeSecretShare
    bn::Fr secret_share, secret_share1;
    ComputeShares(secret_share, secret_share1, vec_a, vec_a, index);
    EXPECT_EQ(secret_share, s_i);
    EXPECT_EQ(secret_share1, s_i);

    values[index] = s_i;
    values2.emplace_back(s_i);
  }

  EXPECT_EQ(vec_a[0], ComputeZi(member_set, values));
  EXPECT_EQ(vec_a, InterpolatePolynom(points, values2));
}

TEST(MclDkgTests, Signing)
{
  mcl::bn::initPairing();

  uint32_t committee_size = 5;
  uint32_t threshold      = 3;
  bn::G2   group_g;
  SetGenerator(group_g);

  // Construct polynomial of degree threshold - 1
  std::vector<bn::Fr> vec_a;
  Init(vec_a, threshold);
  for (std::size_t ii = 0; ii <= threshold; ++ii)
  {
    vec_a[ii].setRand();
  }

  PublicKey group_public_key;
  group_public_key.clear();

  std::string                             message = "Hello";
  std::unordered_map<uint32_t, Signature> threshold_signatures;

  // Generate random selection of committee members
  std::set<uint32_t> members;
  while (members.size() < threshold)
  {
    members.insert(static_cast<uint32_t>(rand()) % committee_size);
  }

  // Group secret key is polynomial evaluated at 0
  PrivateKey group_private_key = vec_a[0];
  bn::G2::mul(group_public_key, group_g, group_private_key);

  // Compute group signature and check validity
  Signature group_sig = SignShare(message, group_private_key);
  EXPECT_EQ(true, VerifySign(group_public_key, message, group_sig, group_g));

  // Generate committee public keys from their private key contributions
  for (uint32_t i = 0; i < committee_size; ++i)
  {
    PrivateKey pow, tmpF, private_key;
    // Private key is polynomial evaluated at index i
    private_key = vec_a[0];
    for (uint32_t k = 1; k < vec_a.size(); k++)
    {
      bn::Fr::pow(pow, i + 1, k);        // adjust index in computation
      bn::Fr::mul(tmpF, pow, vec_a[k]);  // j^k * a_i[k]
      bn::Fr::add(private_key, private_key, tmpF);
    }
    // Public key from private
    PublicKey public_key;
    public_key.clear();
    bn::G2::mul(public_key, group_g, private_key);

    // Compute signature share and validate
    Signature sig = SignShare(message, private_key);
    EXPECT_EQ(true, VerifySign(public_key, message, sig, group_g));

    // Accumulate signature shares from some random threshold number of members
    if (members.find(i) != members.end())
    {
      threshold_signatures.insert({i, sig});
    }
  }

  // Compute group signature from combining signature shares and validate
  Signature group_signature = LagrangeInterpolation(threshold_signatures);
  EXPECT_EQ(true, VerifySign(group_public_key, message, group_signature, group_g));
}
