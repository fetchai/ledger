#ifndef SWARM_SERVICE_HPP
#define SWARM_SERVICE_HPP

#include"service/server.hpp"
#include"network/tcp_server.hpp"
#include"protocols.hpp"
#include"commandline/parameter_parser.hpp"

#include"http/server.hpp"
#include"http/middleware/allow_origin.hpp"
#include"http/middleware/color_log.hpp"

#include<set>

class FetchSwarmService : public fetch::protocols::SwarmProtocol  
{
public:
  FetchSwarmService(uint16_t port, uint16_t http_port, std::string const& pk,
    fetch::network::ThreadManager *tm ) :
    fetch::protocols::SwarmProtocol( tm,  fetch::protocols::FetchProtocols::SWARM, details_),
    thread_manager_( tm ),
    service_(port, thread_manager_),
    http_server_(http_port, thread_manager_)
  {
    using namespace fetch::protocols;
    
    std::cout << "Listening for peers on " << (port) << ", clients on " << (http_port ) << std::endl;

    details_.with_details([=]( NodeDetails &  details) {
        details.public_key = pk;
        details.default_port = port;
        details.default_http_port = http_port;
      });

    EntryPoint e;
    // At this point we don't know what the IP is, but localhost is one entry point    
    e.host = "127.0.0.1"; 
    e.shard = 0;

    e.port = details_.default_port();
    e.http_port = details_.default_http_port();
    e.configuration = EntryPoint::NODE_SWARM;      
    details_.AddEntryPoint(e); 

    running_ = false;
    
    start_event_ = thread_manager_->OnAfterStart([this]() {
        running_ = true;        
        thread_manager_->io_service().post([this]() {
            this->UpdatePeerDetails();
          });
      });

    stop_event_ = thread_manager_->OnBeforeStop([this]() {
        running_ = false;
      });
    
    

    service_.Add(fetch::protocols::FetchProtocols::SWARM, this);

    // Setting callback to resolve IP
    this->SetClientIPCallback([this](uint64_t const &n) -> std::string {
        return service_.GetAddress(n);        
      });

    // Creating a http server based on the swarm protocol
    http_server_.AddMiddleware( fetch::http::middleware::AllowOrigin("*") );       
    http_server_.AddMiddleware( fetch::http::middleware::ColorLog);
    http_server_.AddModule(*this);
    
  }

  ~FetchSwarmService() 
  {

  }

/*                                                
 *  Connectivity maintenance                      
 *  ═══════════════════════════════════════════ 
 *
 *  The swarm node continuously updates the       
 *  connectivity to other nodes and ensure that   
 *  shards are connected to peers. This is done   
 *  through following event loop:                 
 *  ┌─────────────────────────────────────────┐   
 *  │           Update Peer Details           │◀─┐
 *  └────────────────────┬────────────────────┘  │
 *                       │                       │
 *  ┌────────────────────▼────────────────────┐  │
 *  │               Track peers               │  │
 *  └────────────────────┬────────────────────┘  │
 *                       │                       │
 *  ┌────────────────────▼────────────────────┐  │
 *  │        Update shard connectivity        │──┘
 *  └─────────────────────────────────────────┘   
 */
  
  void UpdatePeerDetails() 
  {
    using namespace fetch::protocols;

    fetch::logger.Highlight("Starting Update Connectivity Loop");
    

    NodeDetails details;    
    this->with_node_details( [&details](NodeDetails const &d) {
        details = d;
      });    


    // Updating outgoing details
    bool did_update = false;
    std::map< fetch::byte_array::ByteArray, NodeDetails > all_details;

    
    
    this->with_peers_do( [&all_details, &did_update, details](std::vector< client_shared_ptr_type >  const &peers,
        std::map< uint64_t, NodeDetails >& peer_details) {
        for(auto &c: peers) {
          auto p = c->Call(FetchProtocols::SWARM, SwarmRPC::HELLO, details);          
          
          if(!p.Wait(2000)) {
            fetch::logger.Error("Peer connectivity failed! TODO: Trim connections and inform shards");
            TODO_FAIL("Peer connectivity failed! TODO: Trim connections and inform shards");
          }
          
          auto ref = p.As< NodeDetails >();
          auto &d = peer_details[c->handle()];
          all_details[ref.public_key] = ref;
          
          if( true || (d != ref) ) {
            did_update = true;
            d = ref;
            
            fetch::logger.Highlight("Got update for: ",  d.public_key);
            for(auto &e: d.entry_points) {
              fetch::logger.Debug(" - ", e.host, ":", e.port, ", shard ", e.shard);
            }
          }
        }
        
      });
    

    // Fetching all incoming details
    this->with_client_details_do( [&all_details](std::map< uint64_t, NodeDetails > const &node_details)  {
        for(auto const&d: node_details)
        {
          all_details[d.second.public_key] = d.second;
        }
      });
    
    // Updating all suggestions
    all_details[ details.public_key ] = details;    
    this->with_suggestions_do([&all_details](std::vector< NodeDetails >  & list) {
        fetch::logger.Highlight("Updating suggestions");            
        for(std::size_t i=0; i < list.size(); ++i) {

          auto &details = list[i];
          fetch::logger.Debug(" - updating ", details.public_key);
          for(auto &e: details.entry_points) {
            fetch::logger.Debug("   > ", e.host,":", e.port);            
          }

          
          if(all_details.find(details.public_key) != all_details.end())
          {
            if(details != all_details[ details.public_key ] )
            {
              fetch::logger.Highlight("Updating suggestions info");
              list[i] = details;
              TODO("Proapgate change");              
              // TODO: Propagate change?
            } 
          }
          
        }
      });
    
    // Next we track peers 
    if(running_) {
      thread_manager_->io_service().post([this]() {
          this->TrackPeers();   
        });    
    }    
  }
  
  void TrackPeers() 
  {
    using namespace fetch::protocols;
    
    std::this_thread::sleep_for(std::chrono::milliseconds( 2000 ) );
    std::set< fetch::byte_array::ByteArray > public_keys;    
    public_keys.insert(this->details_.details().public_key);
    

    // Finding keys to those we are connected to
    this->with_server_details_do([&](std::map< uint64_t, NodeDetails > const & details) {
        for(auto const &d: details)
        {
          public_keys.insert( d.second.public_key );          
        }
        
      });

    this->with_client_details_do([&](std::map< uint64_t, NodeDetails > const & details) {
        for(auto const &d: details)
        {
          public_keys.insert( d.second.public_key );          
        }        
      });

    
    // Finding hosts we are not connected to
    std::vector< EntryPoint > swarm_entries;    
    this->with_suggestions_do([=, &swarm_entries](std::vector< NodeDetails > const &details) {
        for(auto const &d: details)
        {
          if( public_keys.find( d.public_key ) == public_keys.end() )
          {
            for(auto const &e: d.entry_points)
            {
              if(e.configuration & EntryPoint::NODE_SWARM)
              {
                swarm_entries.push_back( e );
              }
            }
          }
        }
      });

    std::random_shuffle(swarm_entries.begin(), swarm_entries.end());    
    std::cout << "I wish to connect to: " << std::endl;
    std::size_t i = public_keys.size();

    std::size_t desired_connectivity_  = 5;   
    for(auto &e : swarm_entries) {
      std::cout << " - " << e.host << ":" << e.port << std::endl;
      this->Bootstrap(e.host, e.port);
      
      ++i;      
      if(i > desired_connectivity_ )
      {
        break;        
      }
    }
    
    if(running_) {
      thread_manager_->io_service().post([this]() {
          this->UpdateShardConnectivity();          
        });    
    }    
    
  }

  void UpdateShardConnectivity() 
  {
    using namespace fetch::protocols;
    
    std::vector< EntryPoint > shard_entries;    
    this->with_suggestions_do([=, &shard_entries](std::vector< NodeDetails > const &details) {
        for(auto const &d: details)
        {
          for(auto const &e: d.entry_points)
          {
            if(e.configuration & EntryPoint::NODE_SHARD)
            {
              shard_entries.push_back( e );
            }
          }
        }
      });

    fetch::logger.Highlight("Updating shards!");
    for(auto &s : shard_entries) {
      fetch::logger.Debug(" - ", s.host, ":", s.port);
    }
    
    std::random_shuffle(shard_entries.begin(), shard_entries.end());
    std::vector< client_shared_ptr_type > shards;
    std::vector< EntryPoint > details;
    
    
    this->with_shards_do([this, &shards, &details](std::vector< client_shared_ptr_type > const &sh,
          std::vector< EntryPoint > &det ) {
        std::size_t i =0;
        
        for(auto &s: sh)
        {
          shards.push_back(s);
          details.push_back(det[i]);          
          ++i;          
        }
      });
    
    std::cout << "Updating shards: " << std::endl;          
    for(std::size_t i=0; i < shards.size(); ++i)
    {
      auto client = shards[i];
      auto p1 = client->Call(FetchProtocols::SHARD,  ShardRPC::COUNT_OUTGOING_CONNECTIONS);
      auto p2 = client->Call(FetchProtocols::SHARD,  ShardRPC::SHARD_NUMBER);

      if(! p1.Wait(2300) )
      {
        continue;        
      }
      
      if(! p2.Wait(2300) )
      {
        continue;        
      }
      
      
      uint32_t conn_count = uint32_t( p1  );
      uint32_t shard =  uint32_t( p2  );
      details[i].shard = shard;
      
      std::cout << "  - "<< i << " : " << details[i].host << " " << details[i].port << " " << shard <<" " << conn_count <<  std::endl;      
      // TODO: set shard detail
      

      if(conn_count < 2) // TODO: Desired connectivity;
      {
        for(auto &s: shard_entries)
        {
          std::cout << "Trying to connect " << s.host << ":" << s.port << std::endl;
          
          if( s.shard == shard )
          {
            fetch::logger.Warn("Before call");
            
            client->Call(FetchProtocols::SHARD, ShardRPC::LISTEN_TO, s);
            fetch::logger.Warn("After call");
            
            ++conn_count;
            if(conn_count == 2) // TODO: desired connectivity
            {
              break;                    
            }
            
          }
          
        }
      }

    }

    // Updating shard details
    this->with_shards_do([this, &shards, &details](std::vector< client_shared_ptr_type > const &sh,
          std::vector< EntryPoint > &det ) {
        for(std::size_t i =0; i < details.size(); ++i) {
          if(i < det.size() )
          {
            det[i] = details[i];            
          }          
        }
      });
    
    if(running_) {
      thread_manager_->io_service().post([this]() {
          this->UpdatePeerDetails();          
        });    
    }    
    
  }
  

private:
  fetch::network::ThreadManager *thread_manager_;    
  fetch::service::ServiceServer< fetch::network::TCPServer > service_;
  fetch::http::HTTPServer http_server_;  
  
//  fetch::protocols::SwarmProtocol *swarm_ = nullptr;
  fetch::protocols::SharedNodeDetails details_;

  typename fetch::network::ThreadManager::event_handle_type start_event_;
  typename fetch::network::ThreadManager::event_handle_type stop_event_;  
  std::atomic< bool > running_;  
};

#endif
