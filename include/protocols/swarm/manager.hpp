#ifndef SWARM_MANAGER_HPP
#define SWARM_MANAGER_HPP
#include "service/client.hpp"
#include "service/publication_feed.hpp"
#include "protocols/swarm/node_details.hpp"
#include "protocols/swarm/shard_details.hpp"
#include "protocols/swarm/serializers.hpp"

#include<unordered_set>
#include<atomic>
namespace fetch
{
namespace protocols 
{

class SwarmManager : public fetch::service::HasPublicationFeed 
{
public:
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;  
  
  SwarmManager(uint64_t const&protocol,
    network::ThreadManager *thread_manager,
    NodeDetails  &details)
    :
    protocol_(protocol),
    thread_manager_(thread_manager),
    details_(details),
    sharding_parameter_(0) {    
  }
  
  uint64_t Ping() 
  {
    std::cout << "PING" << std::endl;
    
    return 1337;
  }

  NodeDetails Hello(uint64_t client, NodeDetails details) 
  {
    client_details_[client] = details;
    return details_;
  }

  std::vector< NodeDetails > SuggestPeers() 
  {
    return peers_with_few_followers_;
  }

  void RequestPeerConnections( NodeDetails details ) 
  {
    
    if(details.public_key() == details_.public_key()) 
    {
      std::cout << "Discovered myself" << std::endl;
    }
    else 
    {

      if(already_seen_.find( details.public_key() ) == already_seen_.end())
      {
        std::cout << "Discovered " << details.public_key() << std::endl;
        peers_with_few_followers_.push_back(details);
        already_seen_.insert( details.public_key() );
        this->Publish(SwarmFeed::FEED_REQUEST_CONNECTIONS, details);
        
        for(auto &client: peers_)
        {
          client->Call(protocol_, SwarmRPC::REQUEST_PEER_CONNECTIONS, details);
        }
        
      }
      else
      {
        std::cout << "Ignored " << details.public_key() << std::endl;
      }
      
       
    }
    

  }
  
  void EnoughPeerConnections( NodeDetails details ) 
  {
    bool found = false;
    auto it = peers_with_few_followers_.end();
    while( it != peers_with_few_followers_.begin() ) 
    {
      --it;
      if( (*it) == details ) 
      {
        found = true;
        peers_with_few_followers_.erase( it );
      }
    }
    
    if(found) 
    {
      this->Publish(SwarmFeed::FEED_ENOUGH_CONNECTIONS, details);
    }
  }

  std::string GetAddress(uint64_t client) 
  {
    if(request_ip_) return request_ip_(client);    

    return "unknown";    
  }


  void SetShardingParameter(uint16_t n) 
  {    
    sharding_parameter_ = n;    
  }

  uint16_t GetShardingParameter()   
  {
    return sharding_parameter_;
  }
    
  
  ////////////////////////
  // Not service protocol
  void ConnectShard(std::string const &host, uint16_t const &port ) 
  {
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_);
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Make variable
    shards_mutex_.lock();
    ShardDetails d;
    d.handle = client->handle();
    d.entry_for_swarm.host = host;
    d.entry_for_swarm.port = port;
    d.entry_for_swarm.http_port = -1; // TODO: Request
    d.entry_for_swarm.shard = 0; // TODO: set;
    d.entry_for_swarm.configuration = 0;  
    shards_.push_back(client);
    shards_details_.push_back(d);    
    shards_mutex_.unlock();
  }  

  
  void SetClientIPCallback( std::function< std::string(uint64_t) > request_ip ) 
  {
    request_ip_ = request_ip;    
  }
  
  client_shared_ptr_type Connect( std::string const &host, uint16_t const &port ) 
  {
    using namespace fetch::service;    
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_ );

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Make variable
    

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
      if(already_seen_.find( o.public_key() ) == already_seen_.end()) {
        already_seen_.insert( o.public_key() );        
        peers_with_few_followers_.push_back( o );
      }
      
    }

  }

  void with_shard_details_do(std::function< void(std::vector< ShardDetails > const &) > fnc) 
  {
    shards_mutex_.lock();    
    fnc( shards_details_ );
   
    shards_mutex_.unlock();    
  }

  void with_shards_do(std::function< void(std::vector< client_shared_ptr_type > const &, std::vector< ShardDetails > const &) > fnc) 
  {
    shards_mutex_.lock();    
    fnc( shards_, shards_details_ );
    shards_mutex_.unlock();    
  }

  void with_shards_do(std::function< void(std::vector< client_shared_ptr_type > const &) > fnc) 
  {
    shards_mutex_.lock();    
    fnc( shards_ );   
    shards_mutex_.unlock();    
  }
  
  
  
  void with_suggestions_do(std::function< void(std::vector< NodeDetails > const &) > fnc) 
  {
    fnc( peers_with_few_followers_ );    
  }

  void with_client_details_do(std::function< void(std::map< uint64_t, NodeDetails > const &) > fnc) 
  {
    fnc( client_details_ );    
  }



  void with_peers_do( std::function< void(std::vector< client_shared_ptr_type >  ) > fnc ) 
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
  uint64_t protocol_;
  network::ThreadManager *thread_manager_;  
  NodeDetails &details_;

  
  std::vector< NodeDetails > peers_with_few_followers_;
  std::map< uint64_t,  NodeDetails > client_details_;  
  std::function< std::string(uint64_t) > request_ip_;
  std::unordered_set< std::string > already_seen_;

  std::map< uint64_t, NodeDetails > server_details_;
  std::vector< client_shared_ptr_type > peers_;
  fetch::mutex::Mutex peers_mutex_;

  std::vector< client_shared_ptr_type > shards_;
  std::vector< ShardDetails > shards_details_;
  
  fetch::mutex::Mutex shards_mutex_;  

  std::atomic< uint16_t > sharding_parameter_ ;
};


};
};
#endif 
