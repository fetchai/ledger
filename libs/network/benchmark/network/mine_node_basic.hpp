#pragma once

// This represents the API to the network test
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <set>
#include <stdlib.h>
#include <utility>
#include <vector>

#include "./network_classes.hpp"
#include "./node_directory.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"

#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"

namespace fetch {
namespace network_mine_test {

class MineNodeBasic
{

  // Main chain
  using BlockType = chain::MainChain::BlockType;
  using BlockHash = chain::MainChain::BlockHash;
  using body_type = chain::MainChain::BlockType::body_type;
  using miner     = fetch::chain::consensus::DummyMiner;

public:
  explicit MineNodeBasic(network::NetworkManager tm, uint64_t minerNumber)
    : nodeDirectory_{tm}, minerNumber_{minerNumber}
  {}

  MineNodeBasic(MineNodeBasic &rhs)  = delete;
  MineNodeBasic(MineNodeBasic &&rhs) = delete;
  MineNodeBasic operator=(MineNodeBasic &rhs) = delete;
  MineNodeBasic operator=(MineNodeBasic &&rhs) = delete;

  ~MineNodeBasic() {}

  ///////////////////////////////////////////////////////////
  // RPC calls
  void ReceiveNewHeader(BlockType &block)
  {
    block.UpdateDigest();

    // Verify the block
    block.proof().SetTarget(target_);

    if (!block.proof()())
    {
      fetch::logger.Warn("Received not verified");
    }
    else
    {
      mainChain.AddBlock(block);

      // The main chain will set whether that block was loose. If it was, try
      // and walk down until it touches the main chain
      if (block.loose())
      {
        std::thread{[this, block] {
          BlockType copy = block;
          this->SyncBlock(copy);
        }}
            .detach();
      }
    }
  }

  // Called async. when we see a new block that's loose, work to connect it to
  // the main chain
  void SyncBlock(BlockType &block)
  {
    BlockType walkBlock;
    BlockHash hash = block.body().previous_hash;

    do
    {
      bool success = nodeDirectory_.GetHeader(hash, walkBlock);
      if (!success) break;

      walkBlock.UpdateDigest();  // critical we update the hash after transmission
      hash = walkBlock.body().previous_hash;

    } while (mainChain.AddBlock(walkBlock));
  }

  // Nodes will provide each other with headers
  std::pair<bool, BlockType> ProvideHeader(BlockHash hash)
  {
    BlockType block;
    bool      success = mainChain.Get(hash, block);

    return std::make_pair(success, block);
  }

  ///////////////////////////////////////////////////////////
  // HTTP calls for setup
  void AddEndpoint(const network_benchmark::Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    fetch::logger.Info("Adding endpoint");
    nodeDirectory_.AddEndpoint(endpoint);
  }

  void reset()
  {
    fetch::logger.Info("Resetting miner");
    mainChain.reset();
    stopped_ = true;
  }

  ///////////////////////////////////////////////////////////
  // Mining loop
  void startMining()
  {
    auto closure = [this] {
      // Loop code
      while (!stopped_)
      {
        // Get heaviest block
        auto &block = mainChain.HeaviestBlock();

        // Create another block sequential to previous
        BlockType nextBlock;
        body_type nextBody;
        nextBody.block_number  = block.body().block_number + 1;
        nextBody.previous_hash = block.hash();
        nextBody.miner_number  = minerNumber_;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        // Mine the block
        nextBlock.proof().SetTarget(target_);
        miner::Mine(nextBlock);

        if (stopped_)
        {
          break;
        }

        // Add the block
        mainChain.AddBlock(nextBlock);

        // Pass the block to other miners
        nodeDirectory_.PushBlock(nextBlock);
      }
    };

    std::thread{closure}.detach();
  }

  void stopMining()
  {
    fetch::logger.Info("Stopping mining");
    stopped_ = true;
  }

  ///////////////////////////////////////////////////////////////
  // HTTP functions to check that synchronisation was successful
  std::vector<BlockType> HeaviestChain() { return mainChain.HeaviestChain(); }

  // std::pair<BlockType, std::vector<std::vector<BlockType>>> AllChain()
  //{
  //  return mainChain.AllChain();
  //}

private:
  network_benchmark::NodeDirectory nodeDirectory_;  // Manage connections to other nodes
  fetch::mutex::Mutex              mutex_;
  bool                             stopped_{false};
  std::size_t                      target_ = 16;  // 16 = roughly one block every 0.18s
  uint64_t                         minerNumber_{1};

  chain::MainChain mainChain{uint32_t(minerNumber_)};
};
}  // namespace network_mine_test
}  // namespace fetch
