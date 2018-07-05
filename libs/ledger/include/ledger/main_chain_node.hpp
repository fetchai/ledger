#ifndef MAIN_CHAIN_NODE_HPP
#define MAIN_CHAIN_NODE_HPP

#include <iostream>
#include <string>

#include "ledger/chain/main_chain.hpp"
#include "network/protocols/mainchain/protocol.hpp"
#include "network/interfaces/mainchain/main_chain_node_interface.hpp"
#include "network/details/thread_pool.hpp"
namespace fetch
{
namespace ledger
{

  class MainChainNode : MainChainNodeInterface
{
public:

  typedef fetch::chain::MainChain::proof_type proof_type;
  typedef fetch::chain::MainChain::block_type block_type;
  typedef fetch::chain::MainChain::body_type body_type;
  typedef fetch::chain::MainChain::block_hash block_hash;

  MainChainNode(const MainChainNode &rhs)           = delete;
  MainChainNode(MainChainNode &&rhs)           = delete;
  MainChainNode operator=(const MainChainNode &rhs)  = delete;
  MainChainNode operator=(MainChainNode &&rhs) = delete;
  bool operator==(const MainChainNode &rhs) const = delete;
  bool operator<(const MainChainNode &rhs) const = delete;

  std::shared_ptr<MainChain> chain_;
  std::shared_ptr<network::ThreadPool> threadPool_;
  bool stopped_;

  MainChainNode()
  {
    chain_ = std::make_shared<MainChain>();
    threadPool_ = std::make_shared<network::ThreadPool>(5);
    stopped_ = false;
  }

  virtual ~MainChainNode()
  {
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

  void StartMining()
  {
    auto closure = [this]
    {
      // Loop code
      while(!stopped_)
      {
        // Get heaviest block
        auto &block = chain_ -> HeaviestBlock();

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
  }

};

}
}

#endif //MAIN_CHAIN_NODE_HPP
