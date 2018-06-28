#include "ledger/chain/main_chain.hpp"
#include <iostream>
#include "testing/unittest.hpp"

using namespace fetch::chain;
using namespace fetch::byte_array;

int main(int argc, char const **argv) {

  SCENARIO("testing main chain") {
    SECTION("building on main chain") {

      MainChain::block_type block;

      // Set the block number to guarantee non hash collision
      MainChain::block_type::body_type body;
      body.block_number = 1;
      block.SetBody(body);
      block.UpdateDigest();

      MainChain mainChain{block};

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      // Try adding a non-sequential block (prev hash missing)
      MainChain::block_type dummy;
      MainChain::block_type::body_type dummy_body;
      dummy_body.block_number = 2;
      dummy.SetBody(dummy_body);
      dummy.UpdateDigest();

      mainChain.AddBlock(dummy);

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      fetch::byte_array::ByteArray prevHash = block.hash();

      // Add another 3 blocks in order
      for (std::size_t i = 0; i < 3; ++i)
      {
        // Create another block sequential to previous
        MainChain::block_type nextBlock;
        MainChain::block_type::body_type nextBody;
        nextBody.block_number = 2+i;
        nextBody.previous_hash = prevHash;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        mainChain.AddBlock(nextBlock);

        EXPECT(mainChain.HeaviestBlock().hash() == nextBlock.hash());

        prevHash = nextBlock.hash();
      }
    };

    SECTION("Testing for addition of blocks, out of order") {

      MainChain::block_type block;

      // Set the block number to guarantee non hash collision
      MainChain::block_type::body_type body;
      body.block_number = 1;
      block.SetBody(body);
      block.UpdateDigest();

      MainChain mainChain{block};

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      // Try adding a non-sequential block (prev hash missing)
      MainChain::block_type dummy;
      MainChain::block_type::body_type dummy_body;
      dummy_body.block_number = 2;
      dummy.SetBody(dummy_body);
      dummy.UpdateDigest();

      mainChain.AddBlock(dummy);

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      fetch::byte_array::ByteArray prevHash = block.hash();
      std::vector<MainChain::block_type> blocks;

      // Add another 3 blocks in order
      for (std::size_t i = 0; i < 3; ++i)
      {
        // Create another block sequential to previous
        MainChain::block_type nextBlock;
        MainChain::block_type::body_type nextBody;
        nextBody.block_number = 2+i;
        nextBody.previous_hash = prevHash;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        blocks.push_back(nextBlock);

        prevHash = nextBlock.hash();
      }

      for(auto i : blocks)
      {
        mainChain.AddBlock(i);
      }

      EXPECT(mainChain.HeaviestBlock().hash() == prevHash);
    };

    SECTION("Testing adding blocks that are out of order (should work when all have arrived)") {

      /*
      // Create a block (genesis)
      MainChain::block_type block;

      // Set the body of the block (empty for now)
      MainChain::block_type::body_type body;
      block.SetBody(body);
      block.UpdateDigest();

      MainChain                          mainChain{block};
      std::vector<MainChain::block_type> jumbledBlocks;

      for (std::size_t i = 0; i < 3; ++i)
      {
        // Create another block sequential to previous, with random data (merkle tree)
        MainChain::block_type nextBlock;
        MainChain::block_type::body_type nextBody;
        nextBody.previous_hash = mainChain.Tip().hash();
        nextBody.merkle_hash = fetch::byte_array::ByteArray(std::to_string(i));

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        jumbledBlocks.push_back(nextBlock);
      }

      for(auto &i : jumbledBlocks)
      {
        mainChain.AddBlock(i);
      }

      std::cout << "size is now: " << mainChain.size() << std::endl;

      EXPECT(mainChain.size() == 4); */
    };
  };

  return 0;
}
