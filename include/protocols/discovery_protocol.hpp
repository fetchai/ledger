#ifndef DISCOVERY_PROTOCOL_HPP
#define DISCOVERY_PROTOCOL_HPP

#include "service/function.hpp"
#include "service/client.hpp"
#include "protocols.hpp"
#include "discovery_manager.hpp"

namespace fetch
{
namespace protocols
{
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
    protocol_(protocol) 
  {
    using namespace fetch::service;

    auto ping = new CallableClassMember<DiscoveryProtocol, uint64_t()>(this, &DiscoveryProtocol::Ping);
    auto hello = new CallableClassMember<DiscoveryProtocol, NodeDetails()>(this, &DiscoveryProtocol::Hello);
    auto suggest_peers = new CallableClassMember<DiscoveryProtocol, std::vector< NodeDetails >()>(this, &DiscoveryProtocol::SuggestPeers);
    auto req_conn = new CallableClassMember<DiscoveryProtocol, void(NodeDetails)>(this, &DiscoveryProtocol::RequestPeerConnections);
    auto myip = new CallableClassMember<DiscoveryProtocol, std::string(uint64_t)>(Callable::CLIENT_ID_ARG, this, &DiscoveryProtocol::GetAddress);    
    
    this->Expose(DiscoveryRPC::PING, ping);
    this->Expose(DiscoveryRPC::HELLO, hello);
    this->Expose(DiscoveryRPC::SUGGEST_PEERS, suggest_peers); 
    this->Expose(DiscoveryRPC::REQUEST_PEER_CONNECTIONS, req_conn);
    this->Expose(DiscoveryRPC::WHATS_MY_IP, myip);    

    // Using the event feed that
    this->RegisterFeed(DiscoveryFeed::FEED_REQUEST_CONNECTIONS, this);
    this->RegisterFeed(DiscoveryFeed::FEED_ENOUGH_CONNECTIONS, this);
    this->RegisterFeed(DiscoveryFeed::FEED_ANNOUNCE_NEW_COMER, this);    
  }

  
  client_shared_ptr_type Connect( std::string const &host, uint16_t const &port ) 
  {
    using namespace fetch::service;    
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_ );


    auto ping_promise = client->Call(protocol_, DiscoveryRPC::PING);
    if(!ping_promise.Wait( 2000 )) 
    {
      fetch::logger.Error("Client not repsonding - hanging up!");
      return nullptr;      
    }         
    
    client->Subscribe(protocol_, DiscoveryFeed::FEED_REQUEST_CONNECTIONS,
      new service::Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          DiscoveryManager::RequestPeerConnections(details);
        }) );
    
    client->Subscribe(protocol_, DiscoveryFeed::FEED_ENOUGH_CONNECTIONS ,
      new Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          DiscoveryManager::EnoughPeerConnections(details);
        }) );    

    client->Subscribe(protocol_, DiscoveryFeed::FEED_ANNOUNCE_NEW_COMER ,
      new Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          std::cout << "TODO: figure out what to do here" << std::endl;                          
        }) );    
    

    

    uint64_t ping = uint64_t(ping_promise);    

    if(ping == 1337) 
    {
      fetch::logger.Info("Successfully got PONG");      
      peers_.push_back( client ); 

      service::Promise details_promise = client->Call(protocol_, DiscoveryRPC::HELLO);  
      client->Call(protocol_, DiscoveryRPC::REQUEST_PEER_CONNECTIONS, details_);
      service::Promise ip_promise = client->Call(protocol_, DiscoveryRPC::WHATS_MY_IP );
      
      NodeDetails server_details = details_promise.As< NodeDetails >();
      std::string own_ip = ip_promise.As< std::string >();
      std::cout << "My IP is "<< own_ip << std::endl;
      
      // TODO: Put the details for the server some where
    }
    else    
    {
      fetch::logger.Error("Server gave wrong response - hanging up!");
      return nullptr;      
    }
    
    return client;    
  }

  void Bootstrap(std::string const &host, uint16_t const &port ) 
  {
    // TODO: Check that this node qualifies for bootstrapping
    std::cout << " - bootstrapping " << host << " " << port << std::endl;    
    auto client = Connect( host , port );
    if(!client) 
    {
      fetch::logger.Error("Failed in bootstrapping!");
      return;      
    }    

    auto peer_promise =  client->Call(protocol_, DiscoveryRPC::SUGGEST_PEERS);    
        
    std::vector< NodeDetails > others = peer_promise.As< std::vector< NodeDetails > >();

    for(auto &o : others )  
    {
      std::cout << "Consider connecting to " << o.public_key << std::endl; 
    }

  }
private:
  network::ThreadManager *thread_manager_ = nullptr;      
  NodeDetails &details_;  
  std::vector< client_shared_ptr_type > peers_;
  uint64_t protocol_;
};
}; // namespace protocols
}; // namespace fetch

#endif // DISCOVERY_PROTOCOL_HPP
