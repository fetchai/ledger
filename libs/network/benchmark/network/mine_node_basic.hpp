#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "network_classes.hpp"
#include "node_directory.hpp"

#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"

namespace fetch {
namespace network_mine_test {

class MineNodeBasic
{

  // Main chain
  using BlockType = ledger::Block;
  using BlockHash = ledger::Block::Digest;
  using body_type = ledger::Block::Body;
  using miner     = fetch::ledger::consensus::DummyMiner;

public:
  static constexpr char const *LOGGING_NAME = "MineNodeBasic";

  MineNodeBasic()                    = default;
  MineNodeBasic(MineNodeBasic &rhs)  = delete;
  MineNodeBasic(MineNodeBasic &&rhs) = delete;
  ~MineNodeBasic()                   = default;

  MineNodeBasic operator=(MineNodeBasic &rhs) = delete;
  MineNodeBasic operator=(MineNodeBasic &&rhs) = delete;

  ///////////////////////////////////////////////////////////
  // RPC calls
  void ReceiveNewHeader(BlockType &block)
  {
    block.UpdateDigest();

    // Verify the block
    block.proof.SetTarget(target_);

    if (!block.proof())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Received not verified");
    }
    else
    {
      main_chain_.AddBlock(block);

      // The main chain will set whether that block was loose. If it was, try
      // and walk down until it touches the main chain
      if (block.is_loose)
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
    BlockHash hash = block.body.previous_hash;

    for (;;)
    {
      bool success = node_directory_.GetHeader(hash, walkBlock);
      if (!success)
      {
        break;
      }

      walkBlock.UpdateDigest();  // critical we update the hash after transmission
      hash = walkBlock.body.previous_hash;

      auto status = main_chain_.AddBlock(walkBlock);

      if (status != ledger::BlockStatus::ADDED)
      {
        break;
      }
    }
  }

  // Nodes will provide each other with headers
  std::pair<bool, BlockType> ProvideHeader(BlockHash hash)
  {
    auto block = main_chain_.GetBlock(std::move(hash));

    bool const success = static_cast<bool>(block);

    return std::make_pair(success, success ? *block : BlockType{});
  }

  ///////////////////////////////////////////////////////////
  // HTTP calls for setup
  void AddEndpoint(const network_benchmark::Endpoint &endpoint)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<std::mutex> mlock(mutex_);
    FETCH_LOG_INFO(LOGGING_NAME, "Adding endpoint");
    node_directory_.AddEndpoint(endpoint);
  }

  void reset()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Resetting miner");
    stopped_ = true;
  }

  ///////////////////////////////////////////////////////////
  // Mining loop
  void startMining()
  {
    fetch::ledger::consensus::DummyMiner miner;
    auto                                 closure = [this, &miner] {
      // Loop code
      while (!stopped_)
      {
        // Get heaviest block
        auto block = main_chain_.GetHeaviestBlock();

        // Create another block sequential to previous
        BlockType nextBlock;
        body_type nextBody;
        nextBody.block_number  = block->body.block_number + 1;
        nextBody.previous_hash = block->body.hash;

        nextBlock.body = nextBody;
        nextBlock.UpdateDigest();

        // Mine the block
        nextBlock.proof.SetTarget(target_);
        miner.Mine(nextBlock);

        if (stopped_)
        {
          break;
        }

        // Add the block
        main_chain_.AddBlock(nextBlock);

        // Pass the block to other miners
        node_directory_.PushBlock(nextBlock);
      }
    };

    std::thread{closure}.detach();
  }

  void stopMining()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Stopping mining");
    stopped_ = true;
  }

  ///////////////////////////////////////////////////////////////
  // HTTP functions to check that synchronisation was successful
  std::vector<BlockType> HeaviestChain()
  {
    auto heaviest_chain = main_chain_.GetHeaviestChain();

    std::vector<BlockType> output{};
    output.reserve(heaviest_chain.size());

    for (auto const &block : heaviest_chain)
    {
      output.emplace_back(*block);
    }

    return output;
  }

  // std::pair<BlockType, std::vector<std::vector<BlockType>>> AllChain()
  //{
  //  return main_chain_.AllChain();
  //}

private:
  network_benchmark::NodeDirectory node_directory_;  // Manage connections to other nodes
  fetch::mutex::Mutex              mutex_{__LINE__, __FILE__};
  bool                             stopped_{false};
  std::size_t                      target_ = 16;  // 16 = roughly one block every 0.18s

  ledger::MainChain main_chain_{};
};
}  // namespace network_mine_test
}  // namespace fetch
