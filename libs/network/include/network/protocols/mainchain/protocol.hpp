#pragma once

#include <iostream>
#include <string>

#include "commands.hpp"
#include "network/interfaces/mainchain/main_chain_node_interface.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace protocols {

class MainChainProtocol : public fetch::service::Protocol
{
public:
  MainChainProtocol(ledger::MainChainNodeInterface *node) : Protocol()
  {
    this->Expose(ledger::MainChain::GET_HEADER, node,
                 &ledger::MainChainNodeInterface::GetHeader);
    this->Expose(ledger::MainChain::GET_HEAVIEST_CHAIN, node,
                 &ledger::MainChainNodeInterface::GetHeaviestChain);
  }
};

}  // namespace protocols
}  // namespace fetch
