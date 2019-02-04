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

#include "core/byte_array/encoders.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include <gtest/gtest.h>
#include <iostream>
using namespace fetch::ledger::consensus;
using namespace fetch::byte_array;

ProofOfWork Test1(ByteArray tx, uint64_t diff)
{
  ProofOfWork proof(tx);
  proof.SetTarget(diff);
  while (!proof())
  {
    ++proof;
  }
  return proof;
}

bool TestCompare(ByteArray tx, uint64_t diff1, uint64_t diff2)
{
  ProofOfWork proof1(tx), proof2(tx);
  proof1.SetTarget(diff1);
  proof2.SetTarget(diff2);
  while (!proof1())
  {
    ++proof1;
  }
  while (!proof2())
  {
    ++proof2;
  }

  return (proof1.digest() > proof2.digest());
}

TEST(ledger_proof_of_work_gtest, Easy_difficulty)
{
  auto proof = Test1("Hello world", 1);
  EXPECT_LT(proof.digest(), proof.target());

  proof = Test1("FETCH", 1);
  EXPECT_LT(proof.digest(), proof.target());

  proof = Test1("Blah blah", 1);
  EXPECT_LT(proof.digest(), proof.target());
}
TEST(ledger_proof_of_work_gtest, Slightly_hard_difficulty)
{
  auto proof = Test1("Hello world", 10);
  EXPECT_LT(proof.digest(), proof.target());
  proof = Test1("FETCH", 12);
  EXPECT_LT(proof.digest(), proof.target());
  proof = Test1("Blah blah", 15);
  EXPECT_LT(proof.digest(), proof.target());
}

TEST(ledger_proof_of_work_gtest, Comparing)
{
  EXPECT_TRUE(TestCompare("Hello world", 1, 2));
  EXPECT_TRUE(TestCompare("Hello world", 9, 10));
  EXPECT_TRUE(TestCompare("FETCH", 10, 12));
  EXPECT_TRUE(TestCompare("Blah blah", 3, 15));
}