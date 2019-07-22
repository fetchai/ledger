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

using namespace fetch::dkg;
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

  // Miller and Finel exp
  {
    mcl::bn::Fp12 e1, e2;
    mcl::bn::pairing(e1, P, Q);

    mcl::bn::millerLoop(e2, P, Q);
    mcl::bn::finalExp(e2, e2);
    EXPECT_EQ(e1, e2);
  }

  // Precomputed
  {
    mcl::bn::Fp12 e1, e2;
    mcl::bn::pairing(e1, P, Q);
    std::vector<mcl::bn::Fp6> Qcoeff;
    mcl::bn::precomputeG2(Qcoeff, Q);
    mcl::bn::precomputedMillerLoop(e2, P, Qcoeff);
    mcl::bn::finalExp(e2, e2);
    EXPECT_EQ(e1, e2);
  }

  // opsize
  EXPECT_EQ(sizeof(mcl::fp::Unit), 8);
}

TEST(MclDkgTests, ComputeLhsTrivialSubgroup)
{
  // Construct polynomial of degree 2 (threshold = 1)
  uint32_t            threshold = 1;
  std::vector<bn::Fr> vec_a;
  Init(vec_a, threshold + 1);
  std::vector<bn::Fr> vec_b;
  Init(vec_b, threshold + 1);

  // Randomly initialise the values of a but leave b uninitialised
  for (auto &a_i : vec_a)
  {
    a_i.setRand();
  }

  std::vector<bn::G2> coefficients;
  Init(coefficients, threshold + 1);

  bn::G2 group_g, group_h;
  group_g.clear();
  group_h.clear();
  const bn::Fp2 g("1380305877306098957770911920312855400078250832364663138573638818396353623780",
                  "14633108267626422569982187812838828838622813723380760182609272619611213638781");
  bn::mapToG2(group_g, g);
  for (std::size_t ii = 0; ii <= threshold; ++ii)
  {
    coefficients[ii] = ComputeLHS(group_g, group_h, vec_a[ii], vec_b[ii]);
  }

  // Check ComputeLHS with trivial subgroup h
  std::vector<bn::G2> coefficients_test;
  Init(coefficients_test, threshold + 1);
  for (std::size_t jj = 0; jj <= threshold; ++jj)
  {
    bn::G2 tmpG;
    tmpG.clear();
    bn::G2::mul(tmpG, group_g, vec_a[jj]);
    bn::G2::add(coefficients_test[jj], coefficients_test[jj], tmpG);
  }

  EXPECT_EQ(coefficients, coefficients_test);
}

TEST(MclDkgTests, ComputeLhsNonTrivialSubgroup)
{
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
  group_g.clear();
  group_h.clear();
  const bn::Fp2 g("1380305877306098957770911920312855400078250832364663138573638818396353623780",
                  "14633108267626422569982187812838828838622813723380760182609272619611213638781");
  const bn::Fp2 h("6798148801244076840612542066317482178930767218436703568023723199603978874964",
                  "12726557692714943631796519264243881146330337674186001442981874079441363994424");
  bn::mapToG2(group_g, g);
  bn::mapToG2(group_h, h);

  for (std::size_t ii = 0; ii <= threshold; ++ii)
  {
    coefficients[ii] = ComputeLHS(group_g, group_h, vec_a[ii], vec_b[ii]);
  }

  // Check ComputeLHS with trivial subgroup h
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
}
