#include "ledger/chain/main_chain.hpp"
#include <iostream>
#include "testing/unittest.hpp"

using namespace fetch::chain;
using namespace fetch::byte_array;

//ProofOfWork Test(ByteArray tx, uint64_t diff) {
//  ProofOfWork proof(tx);
//  proof.SetTarget(diff);
//  while (!proof()) {
//    ++proof;
//  }
//  return proof;
//}

int main(int argc, char const **argv) {

  SCENARIO("testing main chain") {
    SECTION("building on main chain") {

      MainChain();

      EXPECT(proof.digest() < proof.target());
    };
  };

  return 0;
}
