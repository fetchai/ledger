#ifndef NODE_FUNCTIONALITY_HPP
#define NODE_FUNCTIONALITY_HPP
#include"commands.hpp"
#include"service/client.hpp"
#include"service/publication_feed.hpp"
#include"assert.hpp"

#include<vector>
#include<string>
#include<iostream>
#include<memory>

class NodeToNodeFunctionality : public fetch::service::HasPublicationFeed {
public:
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  
  NodeToNodeFunctionality(fetch::network::ThreadManager *thread_manager) :
    thread_manager_(thread_manager)
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

    connections_.push_back( std::make_shared< client_type >(host, port, thread_manager_ ) );
  }


private:
  fetch::network::ThreadManager *thread_manager_;  
  std::vector< std::string > messages_;
  std::vector< std::shared_ptr< client_type> > connections_;  
};

#endif
