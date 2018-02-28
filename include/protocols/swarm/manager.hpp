#ifndef SWARM_MANAGER_HPP
#define SWARM_MANAGER_HPP
#include "service/client.hpp"
#include "service/publication_feed.hpp"

#include "protocols/fetch_protocols.hpp"
#include "protocols/swarm/node_details.hpp"

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
    SharedNodeDetails  &details)
    :
    protocol_(protocol),
    thread_manager_(thread_manager),
    details_(details),
    sharding_parameter_(1) {
    // Do not modify details_ here as it is not yet initialized.
  }
  
  uint64_t Ping() 
  {
    std::cout << "PING" << std::endl;
    
    return 1337;
  }

  NodeDetails Hello(uint64_t client, NodeDetails details) 
  {
    client_details_[client] = details;
   //    SendConnectivityDetailsToShards(details);    --- TODO Figure out why this dead locks
    return details_.details();
  }

  std::vector< NodeDetails > SuggestPeers() 
  {
    if(need_more_connections() )
    {
      RequestPeerConnections( details_.details() );      
    }

    std::lock_guard< fetch::mutex::Mutex > lock( suggestion_mutex_ );
    
    return peers_with_few_followers_;
  }

  void RequestPeerConnections( NodeDetails details ) 
  {
    NodeDetails me = details_.details();
    
    suggestion_mutex_.lock();
    if(already_seen_.find( details.public_key ) == already_seen_.end())
    {
      std::cout << "Discovered " << details.public_key << std::endl;
      peers_with_few_followers_.push_back(details);
      already_seen_.insert( details.public_key );
      this->Publish(SwarmFeed::FEED_REQUEST_CONNECTIONS, details);
      
      for(auto &client: peers_)
      {
        client->Call(protocol_, SwarmRPC::REQUEST_PEER_CONNECTIONS, details);
      }
      
    }
    else
    {
      std::cout << "Ignored " << details.public_key << std::endl;
    }             

    suggestion_mutex_.unlock();
  }
  
  void EnoughPeerConnections( NodeDetails details ) 
  {
    suggestion_mutex_.lock();
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
    suggestion_mutex_.unlock();
  }

  std::string GetAddress(uint64_t client) 
  {
    if(request_ip_) return request_ip_(client);    

    return "unknown";    
  }


  void IncreaseShardingParameter() 
  {    
    shards_mutex_.lock();
    uint32_t n = sharding_parameter_ << 1;

    fetch::logger.Debug("Increasing shard parameter to ", n);
    
    struct IDClientPair 
    {
      std::size_t index;      
      client_shared_ptr_type client;      
    } ;
    
    std::map< std::size_t, std::vector< IDClientPair > > shard_org;

    // Computing new sharding assignment
    for(std::size_t i=0; i < n; ++i)
    {
      shard_org[i] = std::vector< IDClientPair >();      
    }
        
    for(std::size_t i=0; i < shards_.size(); ++i)
    {
      auto &details = shards_details_[i];
      auto &client = shards_[i];
      uint32_t s = details.shard;
      shard_org[s].push_back( {i, client} );
    }

    for(std::size_t i=0; i < sharding_parameter_; ++i )
    {
      std::vector< IDClientPair > &vec = shard_org[i];
      if( vec.size() < 2 ) {
        TODO("Throw error - cannot perform sharding without more nodes");        
        shards_mutex_.unlock();
        return;        
      }

      std::size_t bucket2 = i + sharding_parameter_;
      std::vector< IDClientPair > &nvec = shard_org[bucket2];

      std::size_t q = vec.size() >> 1; // Diving shard into two groups
      for(; q != 0; --q)
      {
        nvec.push_back( vec.back() );
        vec.pop_back();        
      }      
    }

    // Assigning shard values
    for(std::size_t i=0; i < n; ++i)
    {
      fetch::logger.Debug("Updating shard nodes in shard ", i);
      auto &vec = shard_org[i];      
      for(auto &c: vec)
      {
        shards_details_[ c.index ].shard = i;        
        c.client->Call(FetchProtocols::SHARD, ShardRPC::SET_SHARD_NUMBER, uint32_t(i), uint32_t(n));

      }
    }
        
    sharding_parameter_ = n;    
    shards_mutex_.unlock();
  }

  uint32_t GetShardingParameter()   
  {
    return sharding_parameter_;
  }
    
  
  ////////////////////////
  // Not service protocol
  void ConnectShard(std::string const &host, uint16_t const &port ) 
  {
    fetch::logger.Debug("Connecting to shard ", host, ":", port);

    shards_mutex_.lock();
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_);
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Make variable

    EntryPoint ep = client->Call(fetch::protocols::FetchProtocols::SHARD, ShardRPC::HELLO, host).As< EntryPoint >();    

    details_.AddEntryPoint( ep );    
    shards_.push_back(client);
    shards_details_.push_back(ep);
    fetch::logger.Debug("Total shard count = ", shards_.size());
    shards_mutex_.unlock();

    // TODO: Send connect to suggestions

  }  

  
  void SetClientIPCallback( std::function< std::string(uint64_t) > request_ip ) 
  {
    request_ip_ = request_ip;    
  }
  
  client_shared_ptr_type Connect( std::string const &host, uint16_t const &port ) 
  {
    using namespace fetch::service;
    fetch::logger.Debug("Connecting to server on ", host, " ", port);
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_ );

    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Connection feedback
    

    fetch::logger.Debug("Pinging server to confirm connection.");
    auto ping_promise = client->Call(protocol_, SwarmRPC::PING);

    
    if(!ping_promise.Wait( 2000 )) 
    {
      fetch::logger.Error("Client not repsonding - hanging up!");
      return nullptr;      
    }         

    fetch::logger.Debug("Subscribing to feeds.");    
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

    fetch::logger.Debug("Waiting for peers_mutex.");        
    std::lock_guard< fetch::mutex::Mutex > lock(peers_mutex_);
    fetch::logger.Debug("Waiting for ping.");        
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
      

//      if(need_more_connections() ) {        
      auto mydetails = details_.details();      
      service::Promise details_promise = client->Call(protocol_, SwarmRPC::HELLO, mydetails);  
      client->Call(protocol_, SwarmRPC::REQUEST_PEER_CONNECTIONS, mydetails);
      
      
      // TODO: add mutex
      NodeDetails server_details = details_promise.As< NodeDetails >();
      std::cout << "Setting details for server with handle: " << client->handle() << std::endl;

      
      if(server_details.entry_points.size() == 0) {
        protocols::EntryPoint e2;
        e2.host = client->Address();
        e2.shard = 0;
        e2.port = server_details.default_port;
        e2.http_port = server_details.default_http_port;
        e2.configuration = EntryPoint::NODE_SWARM;
        server_details.entry_points.push_back(e2);        
      }

//      SendConnectivityDetailsToShards(server_details);
      
      server_details_[ client->handle() ] = server_details;
    }
    else    
    {
      fetch::logger.Error("Server gave wrong response - hanging up!");
      return nullptr;      
    }
    
    return client;    
  }

  
  bool need_more_connections() const
  {
    return true;    
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
      std::cout << "Consider connecting to " << o.public_key << std::endl;
      if(already_seen_.find( o.public_key ) == already_seen_.end()) {
        already_seen_.insert( o.public_key );        
        peers_with_few_followers_.push_back( o );
      }
      
    }

  }

  void with_shard_details_do(std::function< void(std::vector< EntryPoint > &) > fnc) 
  {
    shards_mutex_.lock();    
    fnc( shards_details_ );
   
    shards_mutex_.unlock();    
  }

  void with_shards_do(std::function< void(std::vector< client_shared_ptr_type > const &, std::vector< EntryPoint > &) > fnc) 
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
    // TODO: Add mutex
    fnc( client_details_ );    
  }



  void with_peers_do( std::function< void(std::vector< client_shared_ptr_type >  ) > fnc ) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(peers_mutex_);
    
    fnc(peers_);    
  }

  void with_server_details_do(std::function< void(std::map< uint64_t, NodeDetails > const &) > fnc) 
  {
    peers_mutex_.lock();    
    fnc( server_details_ );
    peers_mutex_.unlock();    
  }

  void with_node_details(std::function< void(NodeDetails const &) > fnc ) {
    details_.with_details( fnc );    
  }
private:

  void SendConnectivityDetailsToShards(NodeDetails const &server_details) 
  {
    std::cout << "WAS HERE -WAS HERE -WAS HERE -WAS HERE -WAS HERE -WAS HERE -WAS HERE -WAS HERE -WAS HERE -" << std::endl;
      
    for(auto const &e2: server_details.entry_points )
    {
      fetch::logger.Debug("Testing ", e2.host, ":", e2.port);
      
      if( e2.configuration & EntryPoint::NODE_SHARD )
      {
        shards_mutex_.lock();
        fetch::logger.Debug(" - Shard count = ", shards_.size() );
        for(std::size_t k=0; k < shards_.size(); ++k)
        {
          auto sd = shards_details_[k];
          fetch::logger.Debug(" - Connect ", e2.host, ":", e2.port,  " >> ", sd.host, ":", sd.port, "?");
          
          if(sd.shard == e2.shard )
          {
            std::cout << "       YES!"  <<std::endl;
            auto sc = shards_[k];
            sc->Call(FetchProtocols::SHARD, ShardRPC::LISTEN_TO, e2);
          }           
        }

        shards_mutex_.unlock();
      }        
    }
  }
  

  
  uint64_t protocol_;
  network::ThreadManager *thread_manager_;  

  SharedNodeDetails &details_;


  std::vector< NodeDetails > peers_with_few_followers_;
  std::map< uint64_t,  NodeDetails > client_details_;  
  std::unordered_set< std::string > already_seen_;
  fetch::mutex::Mutex suggestion_mutex_;
  

  std::function< std::string(uint64_t) > request_ip_;

  std::map< uint64_t, NodeDetails > server_details_;
  std::vector< client_shared_ptr_type > peers_;
  fetch::mutex::Mutex peers_mutex_;

  std::vector< client_shared_ptr_type > shards_;
  std::vector< EntryPoint > shards_details_;  
  fetch::mutex::Mutex shards_mutex_;  

  std::atomic< uint32_t > sharding_parameter_ ;
};


};
};
#endif 
