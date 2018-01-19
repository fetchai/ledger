#ifndef NODE_HPP
#define NODE_HPP

#include"remote_protocol.hpp"
#include"node_protocol.hpp"

class FetchService : public fetch::rpc::ServiceServer {
public:
  FetchService(uint16_t port, std::string const&info) : ServiceServer(port) {
    remote_ =  new RemoteProtocol(info);
    node_ = new NodeProtocol();
    remote_->set_node(node_);
    
    this->Add(FetchProtocols::REMOTE_CONTROL, remote_ );
    this->Add(FetchProtocols::PEER_TO_PEER, node_ );    
  }

  ~FetchService() {
    delete remote_;
    delete node_;
  }

  void Tick() {
    node_->Tick();
  }

  void Tock() {
    node_->Tock();
  }  
private:
  RemoteProtocol *remote_;
  NodeProtocol *node_;

};

#endif
