#ifndef PROTOCOLS_MAIN_CHAIN_PROTOCOL_HPP
#define PROTOCOLS_MAIN_CHAIN_PROTOCOL_HPP
#include "network/service/protocol.hpp"
#include<vector>

namespace fetch
{
namespace chain
{

class MainChainProtocol : public fetch::service::Protocol
{
public:
  using block_type = chain::MainChain::block_type;
  using block_hash_type = chain::MainChain::block_hash;   
  
  enum
  {
    GET_HEADER         = 1,
    GET_HEAVIEST_CHAIN = 2,
  };  
  MainChainProtocol(chain::MainChain *node) : Protocol(), chain_(node) {
    this->Expose(GET_HEADER, this,  &chain::MainChainProtocol::GetHeader);
    this->Expose(GET_HEAVIEST_CHAIN, this,  &chain::MainChainProtocol::GetHeaviestChain);

   }

private:
  chain::MainChain *chain_;
  
  std::pair<bool, block_type> GetHeader(block_hash_type const &hash)
  {
    fetch::logger.Debug("GetHeader starting work");
    block_type block;
    if (chain_ -> Get(hash, block))
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

  std::vector<block_type> GetHeaviestChain(uint32_t maxsize)
  {
    std::vector<block_type> results = chain_->HeaviestChain(maxsize);

    fetch::logger.Debug("GetHeaviestChain returning ", results.size(), " of req ", maxsize);

    return results;
  }
};

}
}

#endif //__MAIN_CHAIN_PROTOCOL_HPP
