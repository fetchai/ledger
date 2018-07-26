#pragma once

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

class MainChainNode : public MainChainNodeInterface,
                      public fetch::http::HTTPModule
{
public:
  typedef fetch::chain::MainChain::proof_type            proof_type;
  typedef fetch::chain::MainChain::block_type            block_type;
  typedef fetch::chain::MainChain::block_type::body_type body_type;
  typedef fetch::chain::MainChain::block_hash            block_hash;
  typedef fetch::chain::consensus::DummyMiner            miner;

  MainChainNode(const MainChainNode &rhs) = delete;
  MainChainNode(MainChainNode &&rhs)      = delete;
  MainChainNode &operator=(const MainChainNode &rhs) = delete;
  MainChainNode &operator=(MainChainNode &&rhs)             = delete;
  bool           operator==(const MainChainNode &rhs) const = delete;
  bool           operator<(const MainChainNode &rhs) const  = delete;

  MainChainNode(
      std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore,
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
    HTTPModule::Post("/mainchain", [this](http::ViewParameters const &params,
                                          http::HTTPRequest const &   req) {
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
      if (index >= limit) break;
    }

    script::Variant result = script::Variant::Object();
    result["blocks"]       = blocks;
    result["chainident"]   = chainident_;

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  // *********** These are RPC calling methods returning a typed promise.

  virtual fetch::network::PromiseOf<std::pair<bool, block_type>>
  RemoteGetHeader(const block_hash &                                     hash,
                  std::shared_ptr<network::NetworkNodeCore::client_type> client)
  {
    auto promise = client->Call(protocol_number, MainChain::GET_HEADER, hash);
    return network::PromiseOf<std::pair<bool, block_type>>(promise);
  }

  virtual fetch::network::PromiseOf<std::vector<block_type>>
  RemoteGetHeaviestChain(
      uint32_t                                               maxsize,
      std::shared_ptr<network::NetworkNodeCore::client_type> client)
  {
    auto promise =
        client->Call(protocol_number, MainChain::GET_HEAVIEST_CHAIN, maxsize);
    return network::PromiseOf<std::vector<block_type>>(promise);
  }

  // *********** These are RPC handlers returning serialisable data.

  virtual std::pair<bool, block_type> GetHeader(const block_hash &hash)
  {
    fetch::logger.Debug("GetHeader starting work");
    block_type block;
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

  virtual std::vector<block_type> GetHeaviestChain(uint32_t maxsize)
  {
    std::vector<block_type> results;

    fetch::logger.Debug("GetHeaviestChain starting work ", maxsize);
    auto currentHash = chain_->HeaviestBlock().hash();

    while (results.size() < maxsize)
    {
      block_type block;
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

    fetch::logger.Debug("GetHeaviestChain returning ", results.size(),
                        " of req ", maxsize);

    return results;
  }

  // *********** These utility methods for node owners.

  bool AddBlock(block_type &block)
  {
    chain_->AddBlock(block);
    return block.loose();
  }

  block_type const &HeaviestBlock() const { return chain_->HeaviestBlock(); }

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
        block_type nextBlock;
        body_type  nextBody;
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

