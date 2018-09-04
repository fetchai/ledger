#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iostream>
#include <string>

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"
#include "network/details/thread_pool.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/generics/promise_of.hpp"
#include "network/interfaces/mainchain/main_chain_node_interface.hpp"
#include "network/protocols/mainchain/commands.hpp"
#include "network/protocols/mainchain/protocol.hpp"

namespace fetch {
namespace ledger {

class MainChainNode : public MainChainNodeInterface, public fetch::http::HTTPModule
{
public:
  using BlockType = fetch::chain::MainChain::BlockType;
  using body_type = fetch::chain::MainChain::BlockType::body_type;
  using BlockHash = fetch::chain::MainChain::BlockHash;
  using miner     = fetch::chain::consensus::DummyMiner;

  MainChainNode(const MainChainNode &rhs) = delete;
  MainChainNode(MainChainNode &&rhs)      = delete;
  MainChainNode &operator=(const MainChainNode &rhs) = delete;
  MainChainNode &operator=(MainChainNode &&rhs)             = delete;
  bool           operator==(const MainChainNode &rhs) const = delete;
  bool           operator<(const MainChainNode &rhs) const  = delete;

  MainChainNode(std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore,
                uint32_t minerNumber, uint32_t target, uint32_t chainident)
    : nnCore_(std::move(networkNodeCore))
  {
    chain_       = std::make_shared<fetch::chain::MainChain>(minerNumber);
    threadPool_  = fetch::network::MakeThreadPool(5);
    stopped_     = false;
    minerNumber_ = minerNumber;
    target_      = target;
    chainident_  = chainident;

    nnCore_->AddProtocol(this);
    HTTPModule::Post("/mainchain",
                     [this](http::ViewParameters const &params, http::HTTPRequest const &req) {
                       return this->HttpGetMainchain(params, req);
                     });
    nnCore_->AddModule(this);
  }

  virtual ~MainChainNode() = default;

  http::HTTPResponse HttpGetMainchain(http::ViewParameters const &params,
                                      http::HTTPRequest const &   req)
  {
    auto   chainArray = chain_->HeaviestChain(999);
    size_t limit      = std::min(chainArray.size(), size_t(999));

    script::Variant blocks = script::Variant::Array(limit);
    std::size_t     index  = 0;
    for (auto &i : chainArray)
    {
      script::Variant temp = script::Variant::Object();
      temp["minerNumber"]  = i.body().miner_number;
      temp["blockNumber"]  = i.body().block_number;
      temp["hashcurrent"]  = ToHex(i.hash());
      temp["hashprev"]     = ToHex(i.body().previous_hash);
      blocks[index++]      = temp;
      if (index >= limit)
      {
        break;
      }
    }

    script::Variant result = script::Variant::Object();
    result["blocks"]       = blocks;
    result["chainident"]   = chainident_;

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  // *********** These are RPC calling methods returning a typed promise.

  virtual fetch::network::PromiseOf<std::pair<bool, BlockType>> RemoteGetHeader(
      const BlockHash &hash, std::shared_ptr<network::NetworkNodeCore::client_type> client)
  {
    auto promise = client->Call(protocol_number, MainChain::GET_HEADER, hash);
    return network::PromiseOf<std::pair<bool, BlockType>>(promise);
  }

  virtual fetch::network::PromiseOf<std::vector<BlockType>> RemoteGetHeaviestChain(
      uint32_t maxsize, std::shared_ptr<network::NetworkNodeCore::client_type> client)
  {
    auto promise = client->Call(protocol_number, MainChain::GET_HEAVIEST_CHAIN, maxsize);
    return network::PromiseOf<std::vector<BlockType>>(promise);
  }

  // *********** These are RPC handlers returning serialisable data.

  virtual std::pair<bool, BlockType> GetHeader(const BlockHash &hash)
  {
    fetch::logger.Debug("GetHeader starting work");
    BlockType block;
    if (chain_->Get(hash, block))
    {
      fetch::logger.Debug("GetHeader done");
      return std::make_pair(true, block);
    }
    else
    {
      fetch::logger.Debug("GetHeader not found");
      return std::make_pair(false, block);
    }
  }

  virtual std::vector<BlockType> GetHeaviestChain(uint32_t maxsize)
  {
    std::vector<BlockType> results;

    fetch::logger.Debug("GetHeaviestChain starting work ", maxsize);
    auto currentHash = chain_->HeaviestBlock().hash();

    while (results.size() < maxsize)
    {
      BlockType block;
      if (chain_->Get(currentHash, block))
      {
        results.push_back(block);
        currentHash = block.body().previous_hash;
      }
      else
      {
        break;
      }
    }

    fetch::logger.Debug("GetHeaviestChain returning ", results.size(), " of req ", maxsize);

    return results;
  }

  // *********** These utility methods for node owners.

  bool AddBlock(BlockType &block)
  {
    chain_->AddBlock(block);
    return block.loose();
  }

  BlockType const &HeaviestBlock() const
  {
    return chain_->HeaviestBlock();
  }

  void StartMining()
  {
    auto closure = [this] {
      // Loop code
      while (!stopped_)
      {
        // Get heaviest block
        auto block = chain_->HeaviestBlock();

        // fetch::logger.Info("MINER: Determining heaviest chain as:",
        // block.summarise());

        // Create another block sequential to previous
        BlockType nextBlock;
        body_type nextBody;
        nextBody.block_number = block.body().block_number + 1;

        nextBody.previous_hash = block.hash();
        nextBody.miner_number  = minerNumber_;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        // Mine the block
        nextBlock.proof().SetTarget(target_);
        miner::Mine(nextBlock);

        // fetch::logger.Info("MINER: Mined block:", nextBlock.summarise());

        if (stopped_)
        {
          break;
        }

        // Add the block
        chain_->AddBlock(nextBlock);
        fetch::logger.Debug("Main Chain Node: Mined: ", ToHex(block.hash()));
      }
    };

    threadPool_->Post(closure);
    threadPool_->Start();
  }

private:
  std::shared_ptr<fetch::chain::MainChain>         chain_;
  network::ThreadPool                              threadPool_;
  std::atomic<bool>                                stopped_;
  uint32_t                                         minerNumber_;
  uint32_t                                         target_;
  uint32_t                                         chainident_;
  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;
};

}  // namespace ledger
}  // namespace fetch
