#ifndef PROTOCOLS_NODE_DISCOVERY_HPP
#define PROTOCOLS_NODE_DISCOVERY_HPP
#include"byte_array/referenced_byte_array.hpp"
#include"service/publication_feed.hpp"
#include"service/protocol.hpp"

#include<vector>

namespace fetch {
namespace protocols {
  
enum DiscoveryRPC {
  PING = 1,
  HELLO = 2,  
  SUGGEST_PEERS = 3,
  REQUEST_PEER_CONNECTIONS = 4,
  DISCONNECT_FEED = 6
};
  
enum DiscoveryFeed {
  FEED_REQUEST_CONNECTIONS = 1,
  FEED_ENOUGH_CONNECTIONS = 2,  
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

  bool operator==(NodeDetails const& other) {
    return public_key == other.public_key;
  }
};
};

namespace serializers {
  template< typename T >
  T& Serialize(T& serializer, protocols::EntryPoint const &data) {
    serializer << data.address;
    serializer << data.shard;
    serializer << data.port;
    return serializer;
  }

  template< typename T >
  T& Deserialize(T& serializer, protocols::EntryPoint &data) {
    serializer >> data.address;
    serializer >> data.shard;
    serializer >> data.port;
    return serializer;
  }
  
  
  template< typename T >
  T& Serialize(T& serializer, protocols::NodeDetails const &data) {
    serializer << data.public_key;
    serializer << uint64_t(data.entry_points.size());
    for(auto const &e: data.entry_points) {
      serializer << e;
    }
    return serializer;
  }

  template< typename T >
  T& Deserialize(T& serializer, protocols::NodeDetails &data) {
    serializer >> data.public_key;
    uint64_t size;
    serializer >> size;
    data.entry_points.resize(size);
    for(auto &e: data.entry_points) {
      serializer >> e;
    }
    return serializer;
  }
  
  template< typename T >
  T& Serialize(T& serializer, std::vector< protocols::NodeDetails > const &data) {
    serializer << uint64_t(data.size() );
    for(auto const &e: data) {
      serializer << e;
    }
    return serializer;
  }
  
  template< typename T >
  T& Deserialize(T& serializer, std::vector< protocols::NodeDetails >  &data) {
    uint64_t size;
    serializer >> size;
    data.resize(size);
    for(auto const &e: data) {
      serializer >> e;
    }
    return serializer;
  }
  
};
  
namespace protocols {
  // TODO: Entrypoint serializer
class DiscoveryManager : public fetch::service::HasPublicationFeed {
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
    this->Publish(DiscoveryFeed::FEED_REQUEST_CONNECTIONS, details);
  }
  
  void EnoughPeerConnections( NodeDetails details ) {
    auto it = peers_with_few_followers_.end();
    while( it != peers_with_few_followers_.begin() ) {
      --it;
      if( (*it) == details ) {
        peers_with_few_followers_.erase( it );
      }
    }
    this->Publish(DiscoveryFeed::FEED_ENOUGH_CONNECTIONS, details);    
  }

private:
  NodeDetails *details_;
  std::vector< NodeDetails > peers_with_few_followers_;
};

class DiscoveryProtocol : public DiscoveryManager,
                          public fetch::service::Protocol { 
public:
  DiscoveryProtocol() : DiscoveryManager(), fetch::service::Protocol() {
    using namespace fetch::service;

    auto ping = new CallableClassMember<DiscoveryProtocol, uint64_t()>(this, &DiscoveryProtocol::Ping);
    auto hello = new CallableClassMember<DiscoveryProtocol, NodeDetails()>(this, &DiscoveryProtocol::Hello);
    auto suggest_peers = new CallableClassMember<DiscoveryProtocol, std::vector< NodeDetails >()>(this, &DiscoveryProtocol::SuggestPeers);
    auto req_conn = new CallableClassMember<DiscoveryProtocol, void(NodeDetails)>(this, &DiscoveryProtocol::RequestPeerConnections);
    
    this->Expose(DiscoveryRPC::PING, ping);
    this->Expose(DiscoveryRPC::HELLO, hello);
    this->Expose(DiscoveryRPC::SUGGEST_PEERS, suggest_peers); 
    this->Expose(DiscoveryRPC::REQUEST_PEER_CONNECTIONS, req_conn);


    // Using the event feed that
    this->RegisterFeed(DiscoveryFeed::FEED_REQUEST_CONNECTIONS, this);
    this->RegisterFeed(DiscoveryFeed::FEED_ENOUGH_CONNECTIONS, this);
    this->RegisterFeed(DiscoveryFeed::ANNOUNCE_NEW_COMER, this);    
};
  
};

};
};
#endif
