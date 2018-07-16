#ifndef PROTOCOLS_MAIN_CHAIN_PROTOCOL_HPP
#define PROTOCOLS_MAIN_CHAIN_PROTOCOL_HPP

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

    this->RegisterFeed(ledger::MainChain::BLOCK_PUBLISH, node -> getPublisher());
   }
};

}
}

#endif //__MAIN_CHAIN_PROTOCOL_HPP
