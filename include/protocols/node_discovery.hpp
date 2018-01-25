#ifndef PROTOCOLS_NODE_DISCOVERY_HPP
#define PROTOCOLS_NODE_DISCOVERY_HPP
#include"byte_array/referenced_byte_array.hpp"
#include"service/publication_feed.hpp"
namespace fetch {
namespace protocols {
  
enum DiscoveryRPC {
  PING = 1,
  HELLO = 2,  
  SUGGEST_PEERS = 3,
  REQUEST_PEER_CONNECTIONS = 4,
  ENOUGH_PEER_CONNECTIONS = 5,  
  DISCONNECT_FEED = 6
};

enum DiscoveryDisconnectReason {
  DISCONNECT_REQUESTED = 0x00,
  NETWORK_ERROR = 0x01,
  BREACH_OF_PROTOCOL = 0x02,
  USELESS_PEER = 0x03,
  TOO_MANY_PEERS = 0x04,
  ALREADY_CONNECTED = 0x05,
  INCOMPATIBLE_PROTOCOL = 0x06, 
  INVALID_IDENTITY = 0x07,
  SERVER_QUITTING = 0x08,
  UNEXPECTED_IDENTITY = 0x09,
  CONNECTED_TO_SELF = 0x0a,
  TIMEOUT_ON_MESSAGE = 0x0b
};
  
enum DiscoveryFeed {
  REQUEST_PEER_CONNECTIONS = 1,
  ENOUGH_PEER_CONNECTIONS = 2,  
  ANNOUNCE_NEW_COMER = 3
};

struct EntryPoint  {
  std::string address;
  uint32_t shard = 0;
  uint32_t port = 1337; 
};

struct NodeDetails {
  byte_array::ByteArray public_key;
  std::vector< EntryPoint > entry_points;
};
  
  // TODO: Entrypoint serializer
class BasicPeerToPeerManager : public fetch::service::HasPublicationFeed {
public:
  uint64_t Ping() {
    return 1337;
  }

  NodeDetails Hello() {
    return *details_;
  }

  std::vector< NodeDetails > SuggestPeers() {
    return peers_with_few_followers_;
  }

  void RequestPeerConnections( NodeDetails details ) {
    peers_with_few_followers_.push_back(details);
    this->Publish(DiscoveryFeed::REQUEST_PEER_CONNECTIONS, details);
  }
  
  void EnoughPeerConnections( NodeDetails details ) {
    auto it = peers_with_few_followers_.end();
    while( it != peers_with_few_followers_.begin() ) {
      --it;
      if( (*it) == details ) {
        peers_with_few_followers_.erase( it );
      }
    }
    this->Publish(DiscoveryFeed::ENOUGH_PEER_CONNECTIONS, details);    
  }

  void Disconnect(uint32_t reason) {
    // TODO: Implement some call back functionality
  }
private:
  NodeDetails *details_;
  std::vector< NodeDetails > peers_with_few_followers_;
};

class BasicPeerToPeerProtocol :  public BasicPeerToPeerManager,
                                 public fetch::service::Protocol { 
public:
  NodeProtocol() : NodeFunctionality(),  fetch::service::Protocol() {
    using namespace fetch::service;

    auto send_msg =  new CallableClassMember<NodeFunctionality, void(std::string)>(this, &NodeFunctionality::SendMessage);
    auto get_msgs =  new CallableClassMember<NodeFunctionality, std::vector< std::string >() >(this, &NodeFunctionality::messages);    
      
    this->Expose(PeerToPeerCommands::SEND_MESSAGE, send_msg);
    this->Expose(PeerToPeerCommands::GET_MESSAGES, get_msgs);

    // Using the event feed that
    this->RegisterFeed( PeerToPeerFeed::NEW_MESSAGE, this  );

};
  
};
};
#endif
