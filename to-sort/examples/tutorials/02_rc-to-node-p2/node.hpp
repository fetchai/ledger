#ifndef NODE_HPP
#define NODE_HPP
#include"node_functionality.hpp"
#include"node_protocol.hpp"

class FetchService : public fetch::rpc::ServiceServer {
public:
  FetchService(uint16_t port) : ServiceServer(port) {
    this->Add(FetchProtocols::AEA, new AEAProtocol() );
  }
};

#endif
