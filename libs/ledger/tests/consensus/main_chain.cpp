#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include <iostream>
#include "testing/unittest.hpp"

using namespace fetch::chain;
using namespace fetch::byte_array;

// TODO: (`HUT`) : get these from helper_functions when it's sorted
// Time related functionality
typedef std::chrono::high_resolution_clock::time_point time_point;

time_point TimePoint()
{
  return std::chrono::high_resolution_clock::now();
}

double TimeDifference(time_point t1, time_point t2)
{
  // If t1 before t2
  if(t1 < t2)
  {
    return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1).count();
  }
  return std::chrono::duration_cast<std::chrono::duration<double>> (t1 - t2).count();
}

typedef MainChain::block_type               block_type;
typedef MainChain::block_type::body_type    body_type;
typedef fetch::chain::concensus::DummyMiner miner;

int main(int argc, char const **argv)
{

  SCENARIO("testing main chain")
  {
    SECTION("building on main chain")
    {
      block_type block;

      // Set the block number to guarantee non hash collision
      body_type body;
      body.block_number = 1;
      block.SetBody(body);
      block.UpdateDigest();

      MainChain mainChain{block};

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      // Try adding a non-sequential block (prev hash missing)
      block_type dummy;
      body_type dummy_body;
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
        block_type nextBlock;
        body_type nextBody;
        nextBody.block_number = 2+i;
        nextBody.previous_hash = prevHash;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        mainChain.AddBlock(nextBlock);

        EXPECT(mainChain.HeaviestBlock().hash() == nextBlock.hash());

        prevHash = nextBlock.hash();
      }
    };

    SECTION("Testing for addition of blocks, out of order")
    {

      block_type block;

      // Set the block number to guarantee non hash collision
      body_type body;
      body.block_number = 1;
      block.SetBody(body);
      block.UpdateDigest();

      MainChain mainChain{block};

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      // Try adding a non-sequential block (prev hash missing)
      block_type dummy;
      body_type dummy_body;
      dummy_body.block_number = 2;
      dummy.SetBody(dummy_body);
      dummy.UpdateDigest();

      mainChain.AddBlock(dummy);

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      fetch::byte_array::ByteArray prevHash = block.hash();
      std::vector<block_type> blocks;

      // Add another 3 blocks in order
      for (std::size_t i = 0; i < 3; ++i)
      {
        // Create another block sequential to previous
        block_type nextBlock;
        body_type nextBody;
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

    SECTION("Testing for addition of blocks, with a break")
    {
      block_type block;

      body_type body;
      body.block_number = 1;
      block.SetBody(body);
      block.UpdateDigest();

      MainChain mainChain{block};

      EXPECT(mainChain.HeaviestBlock().hash() == block.hash());

      fetch::byte_array::ByteArray prevHash = block.hash();
      fetch::byte_array::ByteArray topHash = block.hash();

      // Add another 3 blocks in order
      for (std::size_t i = 2; i < 15; ++i)
      {
        // Create another block sequential to previous
        block_type nextBlock;
        body_type nextBody;
        nextBody.block_number = i;
        nextBody.previous_hash = prevHash;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        if(i != 7)
        {
          mainChain.AddBlock(block);
        }
        else
        {
          topHash = block.hash();
        }
        prevHash = nextBlock.hash();
      }

      EXPECT(!(mainChain.HeaviestBlock().hash() == prevHash));
      EXPECT(mainChain.HeaviestBlock().hash() == topHash);
    };

    SECTION("Test mining/proof")
    {

      std::vector<block_type> blocks;
      std::size_t blockIterations = 10;

      for (std::size_t diff = 1; diff < 32; diff <<= 1)
      {
        auto t1 = TimePoint();

        for (std::size_t j = 0; j < blockIterations; ++j)
        {
          block_type block;
          body_type block_body;
          block_body.block_number = j;
          block_body.nonce = 0;
          block.SetBody(block_body);
          block.UpdateDigest();
          block.proof().SetTarget(diff); // Number of zeroes

          miner::Mine(block);

          blocks.push_back(block);
        }

        auto t2 = TimePoint();
        std::cout << "Difficulty: " << diff << ". Block time: "
          << TimeDifference(t2, t1)/blockIterations << std::endl;
      }

      // Verify blocks
      for(auto &i : blocks)
      {
        if(!i.proof()())
        {
          EXPECT(i.proof()() == 1);
        }
      }

    };

    SECTION("Test mining/proof after serialization")
    {
      std::vector<block_type> blocks;

      for (std::size_t j = 0; j < 100; ++j)
      {
        block_type block;
        body_type block_body;
        block_body.block_number = j;
        block_body.nonce = 0;
        block.SetBody(block_body);
        block.UpdateDigest();
        block.proof().SetTarget(2); // Number of zeroes

        miner::Mine(block);

        blocks.push_back(block);
      }

      bool blockVerified = true;

      // Verify blocks
      for(auto &i : blocks)
      {
        fetch::serializers::ByteArrayBuffer arr;
        arr << i;
        arr.Seek(0);
        block_type block;
        arr >> block;
        block.UpdateDigest();       // digest and target not serialized due to trust issues
        block.proof().SetTarget(2);

        if(!block.proof()())
        {
          std::cout << "not verified" << std::endl;
          blockVerified = false;
        }
      }

      EXPECT(blockVerified == true);

    };
  };

  return 0;
}
