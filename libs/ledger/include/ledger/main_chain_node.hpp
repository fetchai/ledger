#ifndef MAIN_CHAIN_NODE_HPP
#define MAIN_CHAIN_NODE_HPP

#include <iostream>
#include <string>

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "network/generics/network_node_core.hpp"
#include "network/protocols/mainchain/protocol.hpp"
#include "network/interfaces/mainchain/main_chain_node_interface.hpp"
#include "network/generics/promise_of.hpp"
#include "network/details/thread_pool.hpp"
#include "network/protocols/mainchain/protocol.hpp"
#include "network/protocols/mainchain/commands.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/byte_array/decoders.hpp"

namespace fetch
{
namespace ledger
{

  class MainChainNode : public MainChainNodeInterface, public fetch::http::HTTPModule
{
public:

  typedef fetch::chain::MainChain::proof_type proof_type;
  typedef fetch::chain::MainChain::block_type block_type;
  typedef fetch::chain::MainChain::block_type::body_type body_type;
  typedef fetch::chain::MainChain::block_hash block_hash;
  typedef fetch::chain::consensus::DummyMiner     miner;

  MainChainNode(const MainChainNode &rhs)           = delete;
  MainChainNode(MainChainNode &&rhs)           = delete;
  MainChainNode operator=(const MainChainNode &rhs)  = delete;
  MainChainNode operator=(MainChainNode &&rhs) = delete;
  bool operator==(const MainChainNode &rhs) const = delete;
  bool operator<(const MainChainNode &rhs) const = delete;

  std::shared_ptr<fetch::chain::MainChain> chain_;
  std::shared_ptr<network::ThreadPool> threadPool_;
  bool stopped_;
  unsigned int minerNumber_;
  unsigned int target_;
  std::shared_ptr<fetch::network::NetworkNodeCore> nnCore_;

  MainChainNode(std::shared_ptr<fetch::network::NetworkNodeCore> networkNodeCore, unsigned int minerNumber, unsigned int target) :
    nnCore_(networkNodeCore)
  {
    chain_ = std::make_shared<fetch::chain::MainChain>();
    threadPool_ = std::make_shared<network::ThreadPool>(5);
    stopped_ = false;
    minerNumber_ = minerNumber;
    target_ = target;

    nnCore_ -> AddProtocol(this);
    HTTPModule::Post(
                    "/mainchain",
                    [this](http::ViewParameters const &params, http::HTTPRequest const &req) \
                    {
                      return this -> HttpGetMainchain(params, req);
                    });
    nnCore_ -> AddModule(this);
  }

  virtual ~MainChainNode()
  {
  }

  http::HTTPResponse HttpGetMainchain(http::ViewParameters const &params,
      http::HTTPRequest const &req)
  {
    auto chainArray = chain_ -> HeaviestChain();

    script::Variant result = script::Variant::Array(chainArray.size());

    std::size_t index = 0;
    for (auto &i : chainArray) {

      script::Variant temp = script::Variant::Object();
      temp["minerNumber"]  = i.body().miner_number;
      temp["blockNumber"]  = i.body().block_number;
      temp["hashcurrent"]         = ToHex(i.hash());
      temp["hashprev"]     = ToHex(i.body().previous_hash);

      result[index++] = temp;
    }

    std::ostringstream ret;
    ret << result;

    return http::HTTPResponse(ret.str());
  }

  virtual fetch::network::PromiseOf<std::pair<bool, block_type>> RemoteGetHeader(const block_hash &hash, std::shared_ptr<network::NetworkNodeCore::client_type> client)
  {
    auto promise = client->Call(protocol_number, MainChain::GET_HEADER, hash);
    return network::PromiseOf<std::pair<bool, block_type>>(promise);
  }

  virtual fetch::network::PromiseOf<std::vector<block_type>> RemoteGetHeaviestChain(unsigned int maxsize, std::shared_ptr<network::NetworkNodeCore::client_type> client)
  {
    auto promise = client->Call(protocol_number, MainChain::GET_HEAVIEST_CHAIN, maxsize);
    return network::PromiseOf<std::vector<block_type>>(promise);
  }

  virtual std::pair<bool, block_type> GetHeader(const block_hash &hash)
  {
    block_type block;
    if (chain_ -> Get(hash, block))
      {
        return std::make_pair(true, block);
      }
    else
      {
        return std::make_pair(false, block);
      }
  }

  virtual std::vector<block_type> GetHeaviestChain(unsigned int maxsize)
  {
    std::vector<block_type> results;

    auto currentHash = chain_ -> HeaviestBlock().hash();

    while(results.size() < maxsize)
      {
        block_type block;
        if (chain_ -> Get(currentHash, block))
        {
          results.push_back(block);
          currentHash = block.body().previous_hash;
        }
        else
        {
          break;
        }
      }
    return results;
  }

  bool AddBlock(block_type &block)
  {
    chain_ -> AddBlock(block);
    return block.loose();
  }

  block_type const &HeaviestBlock() const
  {
    return chain_ -> HeaviestBlock();
  }

  void StartMining()
  {
    auto closure = [this]
    {
      // Loop code
      while(!stopped_)
      {
        // Get heaviest block
        auto block = chain_ -> HeaviestBlock();

        // Create another block sequential to previous
        block_type nextBlock;
        body_type nextBody;
        nextBody.block_number = block.body().block_number + 1;

        nextBody.previous_hash = block.hash();
        nextBody.miner_number = minerNumber_;

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        // Mine the block
        nextBlock.proof().SetTarget(target_);
        miner::Mine(nextBlock);

        if(stopped_)
        {
          break;
        }

        // Add the block
        chain_ -> AddBlock(nextBlock);

        // Pass the block to other miners
        ///nodeDirectory_.PushBlock(nextBlock);
      }
    };

    threadPool_ -> Post(closure);
    threadPool_ -> Start();
  }

};

}
}

#endif //MAIN_CHAIN_NODE_HPP
