#include "ledger/chain/main_chain.hpp"
#include <iostream>
#include "testing/unittest.hpp"

using namespace fetch::chain;
using namespace fetch::byte_array;

int main(int argc, char const **argv) {

  SCENARIO("testing main chain") {
    SECTION("building on main chain") {

      // Create a block (genesis)
      MainChain::block_type block;

      // Set the body of the block (random initialize)
      MainChain::block_type::body_type body;
      body.group_parameter = 1;

      // Blocks need unique body to avoid hash collisions here
      block.SetBody(body);

      MainChain mainChain{block};

      // Create another block sequential to previous
      MainChain::block_type nextBlock;
      MainChain::block_type::body_type nextBody;
      nextBody.previous_hash = block.header();
      nextBody.group_parameter = 2;
      nextBlock.SetBody(nextBody);

      mainChain.AddBlock(nextBlock);

      std::cout << "size is now: " << mainChain.size() << std::endl;

      EXPECT(mainChain.size() == 2);
    };
  };

  return 0;
}
