#ifndef SWARM_PROTOCOL_HPP
#define SWARM_PROTOCOL_HPP

#include "service/function.hpp"
#include "service/client.hpp"
#include "protocols/swarm/commands.hpp"
#include "protocols/swarm/manager.hpp"

namespace fetch
{
namespace protocols
{
class SwarmProtocol : public SwarmManager,
                      public fetch::service::Protocol { 
public:
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;
  
  SwarmProtocol(network::ThreadManager *thread_manager, uint64_t const &protocol, NodeDetails &details) :
    SwarmManager(details),
    fetch::service::Protocol(),
    thread_manager_(thread_manager),     
    details_(details),    
    protocol_(protocol) 
  {
    using namespace fetch::service;

    auto ping = new CallableClassMember<SwarmProtocol, uint64_t()>(this, &SwarmProtocol::Ping);
    auto hello = new CallableClassMember<SwarmProtocol, NodeDetails(uint64_t, NodeDetails)>(Callable::CLIENT_ID_ARG, this, &SwarmProtocol::Hello);
    auto suggest_peers = new CallableClassMember<SwarmProtocol, std::vector< NodeDetails >()>(this, &SwarmProtocol::SuggestPeers);
    auto req_conn = new CallableClassMember<SwarmProtocol, void(NodeDetails)>(this, &SwarmProtocol::RequestPeerConnections);
    auto myip = new CallableClassMember<SwarmProtocol, std::string(uint64_t)>(Callable::CLIENT_ID_ARG, this, &SwarmProtocol::GetAddress);    
    
    this->Expose(SwarmRPC::PING, ping);
    this->Expose(SwarmRPC::HELLO, hello);
    this->Expose(SwarmRPC::SUGGEST_PEERS, suggest_peers); 
    this->Expose(SwarmRPC::REQUEST_PEER_CONNECTIONS, req_conn);
    this->Expose(SwarmRPC::WHATS_MY_IP, myip);    

    // Using the event feed that
    this->RegisterFeed(SwarmFeed::FEED_REQUEST_CONNECTIONS, this);
    this->RegisterFeed(SwarmFeed::FEED_ENOUGH_CONNECTIONS, this);
    this->RegisterFeed(SwarmFeed::FEED_ANNOUNCE_NEW_COMER, this);    
  }

  
  client_shared_ptr_type Connect( std::string const &host, uint16_t const &port ) 
  {
    using namespace fetch::service;    
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_ );

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    

    auto ping_promise = client->Call(protocol_, SwarmRPC::PING);
    if(!ping_promise.Wait( 2000 )) 
    {
      fetch::logger.Error("Client not repsonding - hanging up!");
      return nullptr;      
    }         
    
    client->Subscribe(protocol_, SwarmFeed::FEED_REQUEST_CONNECTIONS,
      new service::Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          SwarmManager::RequestPeerConnections(details);
        }) );
    
    client->Subscribe(protocol_, SwarmFeed::FEED_ENOUGH_CONNECTIONS ,
      new Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          SwarmManager::EnoughPeerConnections(details);
        }) );    

    client->Subscribe(protocol_, SwarmFeed::FEED_ANNOUNCE_NEW_COMER ,
      new Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          std::cout << "TODO: figure out what to do here" << std::endl;                          
        }) );    
    

    

    uint64_t ping = uint64_t(ping_promise);
    std::lock_guard< fetch::mutex::Mutex > lock(peers_mutex_);
    if(ping == 1337) 
    {
      fetch::logger.Info("Successfully got PONG");      
      peers_.push_back( client ); 

      service::Promise ip_promise = client->Call(protocol_, SwarmRPC::WHATS_MY_IP );
      std::string own_ip = ip_promise.As< std::string >();
      fetch::logger.Info("Node host is ", own_ip);

      // Creating 
      protocols::EntryPoint e;
      e.host = own_ip;
      e.shard = 0;      
      e.port = details_.default_port();
      e.http_port = details_.default_http_port();
      e.configuration = EntryPoint::NODE_SWARM;      
      details_.AddEntryPoint(e); 
      
      
      service::Promise details_promise = client->Call(protocol_, SwarmRPC::HELLO, details_);  
      client->Call(protocol_, SwarmRPC::REQUEST_PEER_CONNECTIONS, details_);

      // TODO: add mutex
      NodeDetails server_details = details_promise.As< NodeDetails >();
      std::cout << "Setting details for server with handle: " << client->handle() << std::endl;

      if(server_details.entry_points().size() == 0) {
        protocols::EntryPoint e2;
        e2.host = client->Address();
        e2.shard = 0;
        e2.port = server_details.default_port();
        e2.http_port = server_details.default_http_port();
        e2.configuration = EntryPoint::NODE_SWARM;
        server_details.AddEntryPoint(e2);        
      }
      
      server_details_[ client->handle() ] = server_details;
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

    auto peer_promise =  client->Call(protocol_, SwarmRPC::SUGGEST_PEERS);    
        
    std::vector< NodeDetails > others = peer_promise.As< std::vector< NodeDetails > >();

    for(auto &o : others )  
    {
      std::cout << "Consider connecting to " << o.public_key() << std::endl; 
    }

  }

  void with_peers_do( std::function< void(std::vector< client_shared_ptr_type > ) > fnc ) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(peers_mutex_);
    
    fnc(peers_);    
  }

  void with_server_details_do(std::function< void(std::map< uint64_t, NodeDetails > const &) > fnc) 
  {
    // TODO: Mutex
    fnc( server_details_ );    
  }
  

private:
  std::map< uint64_t, NodeDetails > server_details_;
  
  network::ThreadManager *thread_manager_ = nullptr;      
  NodeDetails &details_;  
  std::vector< client_shared_ptr_type > peers_;
  fetch::mutex::Mutex peers_mutex_;
  
  uint64_t protocol_;
};
}; // namespace protocols
}; // namespace fetch

#endif // SWARM_PROTOCOL_HPP
