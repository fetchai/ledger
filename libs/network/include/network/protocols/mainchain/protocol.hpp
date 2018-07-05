#ifndef __MAIN_CHAIN_PROTOCOL_HPP
#define __MAIN_CHAIN_PROTOCOL_HPP

#include <iostream>
#include <string>

#include "network/service/protocol.hpp"
#include "commands.hpp"
#include "network/interfaces/mainchain/main_chain_node_interface.hpp"

namespace fetch
{
namespace protocols
{

class MainChainProtocol : public fetch::service::Protocol
{
public:
  MainChainProtocol(ledger::MainChainNodeInterface *node) : Protocol() {
    this->Expose(ledger::MainChain::GET_HEADER,                  node,  &ledger::MainChainNodeInterface::GetHeader);
    this->Expose(ledger::MainChain::GET_HEAVIEST_CHAIN,          node,  &ledger::MainChainNodeInterface::GetHeaviestChain);
   }
};

}
}

#endif //__MAIN_CHAIN_PROTOCOL_HPP
