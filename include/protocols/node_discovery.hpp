#ifndef PROTOCOLS_NODE_DISCOVERY_HPP
#define PROTOCOLS_NODE_DISCOVERY_HPP
#include"byte_array/referenced_byte_array.hpp"

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
  FEED_ANNOUNCE_NEW_COMER = 3
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
    for(auto &e: data) {
      serializer >> e;
    }
    return serializer;
  }
  
};
};


#include"service/publication_feed.hpp"
#include"service/function.hpp"
#include"service/protocol.hpp"
#include"service/client.hpp"
#include"network/tcp_client.hpp"

#include<vector>

namespace fetch  {  
namespace protocols {
  // TODO: Entrypoint serializer
class DiscoveryManager : public fetch::service::HasPublicationFeed {
public:
  DiscoveryManager(NodeDetails const &details)
    : details_(details) { }
  
  uint64_t Ping() {
    std::cout << "PING" << std::endl;
    
    return 1337;
  }

  NodeDetails Hello() {
    return details_;
  }

  std::vector< NodeDetails > SuggestPeers() {
    return peers_with_few_followers_;
  }

  void RequestPeerConnections( NodeDetails details ) {
    peers_with_few_followers_.push_back(details);
    if(details.public_key == details_.public_key) {
      std::cout << "Discovered myself" << std::endl;
    } else {
      std::cout << "Discovered " << details.public_key << std::endl;
    }
    
    this->Publish(DiscoveryFeed::FEED_REQUEST_CONNECTIONS, details);
  }
  
  void EnoughPeerConnections( NodeDetails details ) {
    bool found = false;
    auto it = peers_with_few_followers_.end();
    while( it != peers_with_few_followers_.begin() ) {
      --it;
      if( (*it) == details ) {
        found = true;
        peers_with_few_followers_.erase( it );
      }
    }
    
    if(found) {
      this->Publish(DiscoveryFeed::FEED_ENOUGH_CONNECTIONS, details);
    }
  }

private:
  NodeDetails const &details_;
  std::vector< NodeDetails > peers_with_few_followers_;

};

class DiscoveryProtocol : public DiscoveryManager,
                          public fetch::service::Protocol { 
public:
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;
  
  DiscoveryProtocol(network::ThreadManager *thread_manager, uint64_t const &protocol, NodeDetails &details) :
    DiscoveryManager(details),
    fetch::service::Protocol(),
    thread_manager_(thread_manager),     
    details_(details),    
    protocol_(protocol) {
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
    this->RegisterFeed(DiscoveryFeed::FEED_ANNOUNCE_NEW_COMER, this);    
  }

  
  client_shared_ptr_type Connect( std::string const &host, uint16_t const &port ) {
    using namespace fetch::service;    
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_ );


    auto ping_promise = client->Call(protocol_, DiscoveryRPC::PING);
    if(!ping_promise.Wait( 2000 )) {
      fetch::logger.Error("Client not repsonding - hanging up!");
      return nullptr;      
    }         
    
    client->Subscribe(protocol_, DiscoveryFeed::FEED_REQUEST_CONNECTIONS,
                      new service::Function< void(NodeDetails) >([this](NodeDetails const& details) {
                          DiscoveryManager::RequestPeerConnections(details);
                        }) );
    
    client->Subscribe(protocol_, DiscoveryFeed::FEED_ENOUGH_CONNECTIONS ,
                      new Function< void(NodeDetails) >([this](NodeDetails const& details) {
                          DiscoveryManager::EnoughPeerConnections(details);
                        }) );    

    client->Subscribe(protocol_, DiscoveryFeed::FEED_ANNOUNCE_NEW_COMER ,
                      new Function< void(NodeDetails) >([this](NodeDetails const& details) {
                          std::cout << "TODO: figure out what to do here" << std::endl;                          
                        }) );    
    

    

    uint64_t ping = uint64_t(ping_promise);    

    if(ping == 1337) 
    {
      fetch::logger.Info("Successfully got PONG");      
      peers_.push_back( client ); 

      service::Promise details_promise = client->Call(protocol_, DiscoveryRPC::HELLO);  
      client->Call(protocol_, DiscoveryRPC::REQUEST_PEER_CONNECTIONS, details_);
      
      // TODO: Get own IP
      NodeDetails client_details = details_promise.As< NodeDetails >();
    }
    else
    {
      fetch::logger.Error("Client gave wrong response - hanging up!");
      return nullptr;      
    }
    
    return client;    
  }

  void Bootstrap(std::string const &host, uint16_t const &port ) {
    // TODO: Check that this node qualifies for bootstrapping
    std::cout << " - bootstrapping " << host << " " << port << std::endl;    
    auto client = Connect( host , port );
    if(!client) {
      fetch::logger.Error("Failed in bootstrapping!");
      return;      
    }

    std::cout << "Was here?" << std::endl;
    
    auto peer_promise =  client->Call(protocol_, DiscoveryRPC::SUGGEST_PEERS);
    
        
    std::vector< NodeDetails > others = peer_promise.As< std::vector< NodeDetails > >();

    for(auto &o : others )  {
      std::cout << "Consider connecting to " << o.public_key << std::endl; 
    }

  }
private:
  network::ThreadManager *thread_manager_;      
  NodeDetails &details_;  
  std::vector< client_shared_ptr_type > peers_;
  uint64_t protocol_;
};

};
};
#endif
