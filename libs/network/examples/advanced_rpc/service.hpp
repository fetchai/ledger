#ifndef NODE_HPP
#define NODE_HPP

#include"aea_protocol.hpp"
#include"node_protocol.hpp"

class FetchService {
public:
  FetchService(uint16_t port, std::string const&info) :
    thread_manager_(8),
    service_(port, thread_manager_) {
    
    aea_ =  new AEAProtocol(info);
    node_ = new NodeToNodeProtocol( thread_manager_ );
    
    aea_->set_node(node_);
    
    service_.Add(FetchProtocols::AEA_PROTOCOL, aea_ );
    service_.Add(FetchProtocols::PEER_TO_PEER, node_ );    
  }

  ~FetchService() {
    delete aea_;
    delete node_;
  }

  void Tick() {
    node_->Tick();
  }

  void Tock() {
    node_->Tock();
  }

  void Start() 
  {
    thread_manager_.Start();    
  }

  void Stop() 
  {
    thread_manager_.Stop();    
  }
  
private:  
  fetch::network::ThreadManager thread_manager_;
  
  fetch::service::ServiceServer< fetch::network::TCPServer >  service_;  
  AEAProtocol *aea_;
  NodeToNodeProtocol *node_;

};

#endif
