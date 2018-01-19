#ifndef NODE_PROTOCOL_HPP
#define NODE_PROTOCOL_HPP
#include"vector_serialize.hpp"
#include"node_functionality.hpp"
#include"commands.hpp"

#include"rpc/service_server.hpp"

//template <typename C, typename F>
//class Feed;

//template <typename C, typename R, typename... Args> <C, R(Args...)> :

class NodeProtocol : public NodeFunctionality, public fetch::rpc::Protocol { 
public:
  NodeProtocol() : NodeFunctionality(),  fetch::rpc::Protocol() {

    using namespace fetch::rpc;
    auto send_msg =  new CallableClassMember<NodeFunctionality, void(std::string)>(this, &NodeFunctionality::SendMessage);
    auto get_msgs =  new CallableClassMember<NodeFunctionality, std::vector< std::string >() >(this, &NodeFunctionality::messages);    
      
    this->Expose(PeerToPeerCommands::SEND_MESSAGE, send_msg);
    this->Expose(PeerToPeerCommands::GET_MESSAGES, get_msgs);

    // Using the event feed that
    this->RegisterFeed( PeerToPeerFeed::NEW_MESSAGE, this  );
    
    //    this->create_publisher(PeerToPeerFeed::NEW_MESSAGE, this->get_feed(PeerToPeerFeed::NEW_MESSAGE ) );
    //    AbstractPublicationFeed::create_publisher(PeerToPeerFeed::NEW_MESSAGE, this, &NodeProtocol::Connecting);            
    //    AbstractPublicationFeed::create_publisher(PeerToPeerFeed::CONNECTING, this, &NodeProtocol::Connecting);        
  }
  
  //  void Connecting(fetch::byte_array::ReferencedByteArray const&  msg) {
  //    std::cout << "Connecting here?" << std::endl;
  //  }  
};

#endif
