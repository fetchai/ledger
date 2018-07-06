#ifndef MAIN_CHAIN_NODE_INTERFACE_HPP
#define MAIN_CHAIN_NODE_INTERFACE_HPP

#include "network/protocols/fetch_protocols.hpp"
#include "ledger/chain/main_chain.hpp"


namespace fetch
{
namespace protocols
{
  class MainChainProtocol;
}

namespace ledger
{

class MainChainNodeInterface
{
public:

  typedef fetch::chain::MainChain::proof_type proof_type;
  typedef fetch::chain::MainChain::block_type block_type;
  typedef fetch::chain::MainChain::block_hash block_hash;

  enum {
    protocol_number = fetch::protocols::FetchProtocols::MAIN_CHAIN
  };
  typedef fetch::protocols::MainChainProtocol protocol_class_type;

  MainChainNodeInterface()
  {
  }

  virtual ~MainChainNodeInterface()
  {
  }

  virtual std::pair<bool, block_type>  GetHeader(const block_hash &hash) = 0;
  virtual std::vector<block_type> GetHeaviestChain(unsigned int maxsize) = 0;
};

}
}

#endif //MAIN_CHAIN_NODE_INTERFACE_HPP
