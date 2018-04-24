#include "chain/consensus/proof_of_work.hpp"
#include <iostream>
#include "byte_array/encoders.hpp"
#include "unittest.hpp"
using namespace fetch::chain::consensus;
using namespace fetch::byte_array;

ProofOfWork Test(ByteArray tx, uint64_t diff) {
  ProofOfWork proof(tx);
  proof.SetTarget(diff);
  while (!proof()) {
    ++proof;
  }
  return proof;
}

bool TestCompare(ByteArray tx, uint64_t diff1, uint64_t diff2) {
  ProofOfWork proof1(tx), proof2(tx);
  proof1.SetTarget(diff1);
  proof2.SetTarget(diff2);
  while (!proof1()) {
    ++proof1;
  }
  while (!proof2()) {
    ++proof2;
  }

  return (proof1.digest() > proof2.digest());
}

int main(int argc, char const **argv) {
  SCENARIO("testing proof of work / double SHA") {
    SECTION("Easy difficulty") {
      auto proof = Test("Hello world", 1);
      EXPECT(proof.digest() < proof.target());

      proof = Test("FETCH", 1);
      EXPECT(proof.digest() < proof.target());

      proof = Test("Blah blah", 1);
      EXPECT(proof.digest() < proof.target());
    };

    SECTION("Slightly hard difficulty") {
      auto proof = Test("Hello world", 10);
      EXPECT(proof.digest() < proof.target());
      proof = Test("FETCH", 12);
      EXPECT(proof.digest() < proof.target());
      proof = Test("Blah blah", 15);
      EXPECT(proof.digest() < proof.target());
    };

    SECTION("Comparing") {
      EXPECT(TestCompare("Hello world", 1, 2));
      EXPECT(TestCompare("Hello world", 9, 10));
      EXPECT(TestCompare("FETCH", 10, 12));
      EXPECT(TestCompare("Blah blah", 3, 15));
    };
  };

  return 0;
}
