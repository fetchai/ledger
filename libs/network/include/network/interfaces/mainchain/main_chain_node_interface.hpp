#pragma once

#include "ledger/chain/main_chain.hpp"
#include "network/protocols/fetch_protocols.hpp"

namespace fetch {
namespace protocols {
class MainChainProtocol;
}

namespace ledger {

class MainChainNodeInterface
{
public:
  using BlockType = fetch::chain::MainChain::BlockType;
  using BlockHash = fetch::chain::MainChain::BlockHash;

  enum
  {
    protocol_number = fetch::protocols::FetchProtocols::MAIN_CHAIN
  };
  using protocol_class_type = fetch::protocols::MainChainProtocol;

  MainChainNodeInterface()          = default;
  virtual ~MainChainNodeInterface() = default;

  virtual std::pair<bool, BlockType> GetHeader(const BlockHash &hash)   = 0;
  virtual std::vector<BlockType>     GetHeaviestChain(uint32_t maxsize) = 0;
};

}  // namespace ledger
}  // namespace fetch
