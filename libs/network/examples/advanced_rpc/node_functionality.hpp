#ifndef NODE_FUNCTIONALITY_HPP
#define NODE_FUNCTIONALITY_HPP
#include"commands.hpp"
#include"network/service/client.hpp"
#include"network/service/publication_feed.hpp"
#include"core/assert.hpp"

#include<vector>
#include<string>
#include<iostream>
#include<memory>

class NodeToNodeFunctionality : public fetch::service::HasPublicationFeed {
public:
  typedef fetch::service::ServiceClient client_type;
  
  NodeToNodeFunctionality(fetch::network::NetworkManager network_manager) :
    network_manager_(network_manager)
  {  }

  void Tick() {
    this->Publish(PeerToPeerFeed::NEW_MESSAGE, "tick");
  }
  
  void Tock() {
    this->Publish(PeerToPeerFeed::NEW_MESSAGE, "tock");
  }  
  
  void SendMessage(std::string message) {
   std::cout << "Received message: " << message  << std::endl;
   messages_.push_back(message);
  }

  std::vector< std::string > messages() {
    return messages_;
  }

  void Connect(std::string host, uint16_t port) {
    std::cout << "Node connecting to " << host << " on " << port << std::endl;

    this->Publish(PeerToPeerFeed::CONNECTING, host, port);

    fetch::network::TCPClient connection(network_manager_);
    connection.Connect(host,port);
    
    connections_.push_back( std::make_shared< client_type >(connection, network_manager_ ) );
  }


private:
  fetch::network::NetworkManager network_manager_;  
  std::vector< std::string > messages_;
  std::vector< std::shared_ptr< client_type> > connections_;  
};

#endif
