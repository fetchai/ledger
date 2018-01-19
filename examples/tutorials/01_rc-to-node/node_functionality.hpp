#ifndef NODE_FUNCTIONALITY_HPP
#define NODE_FUNCTIONALITY_HPP
#include"commands.hpp"
#include"rpc/service_client.hpp"
#include"rpc/publication_feed.hpp"
#include"assert.hpp"

#include<vector>
#include<string>
#include<iostream>
#include<memory>

class NodeFunctionality : public fetch::rpc::HasPublicationFeed {
public:
  NodeFunctionality() {  }

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
    connections_.push_back( std::make_shared< fetch::rpc::ServiceClient >(host, port ) );
  }


private:
  std::vector< std::string > messages_;
  std::vector< std::shared_ptr< fetch::rpc::ServiceClient > > connections_;  
};

#endif
