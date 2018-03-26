#ifndef SWARM_CONTROLLER_HPP
#define SWARM_CONTROLLER_HPP
#include "service/client.hpp"
#include "service/publication_feed.hpp"

#include "protocols/fetch_protocols.hpp"
#include "protocols/swarm/node_details.hpp"

#include "protocols/swarm/serializers.hpp"
#include"protocols/chain_keeper/chain_manager.hpp"

#include<unordered_set>
#include<atomic>

namespace fetch
{
namespace protocols 
{
/*
// TODO: To be moved out
class ChainController 
{
public:
  // Block defs  
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef fetch::chain::BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef fetch::chain::BasicBlock<  proof_type, fetch::crypto::SHA256 > block_type;

  ChainController() 
  {
    block_body_type genesis_body;
    block_type genesis_block;
    
    genesis_body.previous_hash = "genesis" ;
    genesis_body.transaction_hashes.push_back("genesis");
    genesis_body.group_parameter = 1;
    genesis_body.groups.push_back(0);
    
    genesis_block.SetBody( genesis_body );

    genesis_block.set_block_number(0);
    
    PushBlock( genesis_block );    
  }
  
  block_type ExchangeHeads(block_type head_candidate) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    fetch::logger.Debug("Entering ", __FUNCTION_NAME__);
    fetch::logger.Debug("Sending head as response to request");
    std::lock_guard< fetch::mutex::Mutex > lock(block_mutex_);
    
    // TODO: Check which head is better
    fetch::logger.Debug("Return!");    
    return *chain_manager_.head();
  }

  std::vector< block_type > GetLatestBlocks(  ) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    return chain_manager_.latest_blocks();    
  }
  
  block_type GetNextBlock() { 
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    block_body_type body;
    block_type block;
    
    block_mutex_.lock();    
    body.previous_hash =  chain_manager_.head()->header();
    body.group_parameter = grouping_parameter_;
      
    if( !tx_manager_.has_unapplied() ) {
      body.transaction_hashes.clear();
    } else {
      auto digest = tx_manager_.NextDigest() ;
      auto const& groups = tx_manager_.Next().groups();
      
      for(auto const &g: groups) {
        body.transaction_hashes.push_back(digest);
        body.groups.push_back(g);
      }
    }
    block_mutex_.unlock();
    
    block.SetBody( body );   
    block.set_total_weight( chain_manager_.head()->total_weight() );
    block.set_block_number( chain_manager_.head()->block_number() + 1 );
    
    
    return block;
    
  }
  
  void PushBlock(block_type block) {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    block_mutex_.lock();
    chain_manager_.AddBlock( block );
    block_mutex_.unlock();

//    VerifyState();
  }

  std::size_t block_count() const 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(block_mutex_);
    return chain_manager_.size();        
  }

  bool AddBulkBlocks(std::vector< block_type > const &new_blocks ) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;    
    std::lock_guard< fetch::mutex::Mutex > lock(block_mutex_);
    return chain_manager_.AddBulkBlocks(new_blocks ) ;    
  }


  void ExchangeHeads() 
  {
        fetch::logger.Debug("Requesting head exchange");    
    auto promise1 = client->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::EXCHANGE_HEADS, head_copy);    
    if(!promise1.Wait(1000) ) { //; // TODO: make configurable
      fetch::logger.Error("Failed to get head.");
      exit(-1);
      
      return;        
    }
    if( promise1.has_failed() ) {
      fetch::logger.Error("Request for head failed.");
      return;        
    }
    
    if( promise1.is_connection_closed() ) {
      fetch::logger.Error("Lost connection.");
      return;           
    }
    
    
    block_type comp_head = promise1.As< block_type >();
    fetch::logger.Debug("Done");
    
//    comp_head.meta_data() = block_meta_data_type();      
    
    PushBlock(comp_head);
  }


  void SetGroupNumber(uint32_t group, uint32_t total_groups) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;

    block_mutex_.lock();
    chain_manager_.set_group( group );    
    block_mutex_.unlock();
  }


  void with_blocks_do( std::function< void(ChainManager::shared_block_type, ChainManager::chain_map_type const& )  > fnc ) const
  {
    block_mutex_.lock();
    fnc( chain_manager_.head(), chain_manager_.chains() );    
    block_mutex_.unlock();
  }

  void with_blocks_do( std::function< void(ChainManager::shared_block_type, ChainManager::chain_map_type & )  > fnc ) 
  {
    LOG_STACK_TRACE_POINT_WITH_INSTANCE;
    
    block_mutex_.lock();
    fnc( chain_manager_.head(), chain_manager_.chains() );    
    block_mutex_.unlock();
  }

  /*
    Protocol::Expose(ChainKeeperRPC::PUSH_BLOCK, push_block);
    Protocol::Expose(ChainKeeperRPC::GET_BLOCKS, get_blocks);        
    Protocol::Expose(ChainKeeperRPC::GET_NEXT_BLOCK, get_block);
    auto push_block = new CallableClassMember<ChainKeeperProtocol, void(block_type) >(this, &ChainKeeperProtocol::PushBlock );
    auto get_block = new CallableClassMember<ChainKeeperProtocol, block_type() >(this, &ChainKeeperProtocol::GetNextBlock );
    auto get_blocks = new CallableClassMember<ChainKeeperProtocol, std::vector< block_type >() >(this, &ChainKeeperProtocol::GetLatestBlocks );

    auto exchange_heads = new CallableClassMember<ChainKeeperProtocol, block_type(block_type) >(this, &ChainKeeperProtocol::ExchangeHeads );
    
    Protocol::Expose(ChainKeeperRPC::EXCHANGE_HEADS, exchange_heads);


    Protocol::RegisterFeed(ChainKeeperFeed::FEED_NEW_BLOCKS, this);    
    
    auto list_blocks = [this](fetch::http::ViewParameters const &params, fetch::http::HTTPRequest const &req) {
      LOG_STACK_TRACE_POINT;
      std::stringstream response;

      auto group_number = this->group_number();
      
      response << "{\"blocks\": [";  
      this->with_blocks_do([group_number, &response](ChainManager::shared_block_type block, ChainManager::chain_map_type & chain) {
          std::size_t i=0;
          while( (i< 10) && ( block  ) ) {
            if(i!=0) response << ", ";                   
            response << "{";
            response << "\"block_hash\": \"" << byte_array::ToBase64( block->header() ) << "\",";
            response << "\"block_hash\": \"" << byte_array::ToBase64( block->header() ) << "\",";
            auto prev = block->body().previous_hash;

            response << "\"previous_hash\": \"" << byte_array::ToBase64( prev  ) << "\",";

            // byte_array::ToBase64( block->body().transaction_hash )
            response << "\"transaction_hash\": \"" << "TODO list" << "\",";            
            response << "\"block_number\": " <<  block->block_number()  << ",";
            response << "\"total_work\": " <<  block->total_weight();            
            response << "}";            
            block = block->previous( );
            ++i; 
          }

        });
      
      response << "]}";

      fetch::logger.Highlight( response.str() );
      
      std::cout << response.str() << std::endl;
            
      return fetch::http::HTTPResponse(response.str());      
    };
    HTTPModule::Get("/list/blocks",  list_blocks);


  
private:
  mutable fetch::mutex::Mutex block_mutex_;
  ChainManager chain_manager_;
  
} ;
*/

class SwarmController : public fetch::service::HasPublicationFeed 
{
public:
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;  
  
  SwarmController(uint64_t const&protocol,
    network::ThreadManager *thread_manager,
    SharedNodeDetails  &details)
    :
    protocol_(protocol),
    thread_manager_(thread_manager),
    details_(details),
    suggestion_mutex_( __LINE__, __FILE__),
    peers_mutex_( __LINE__, __FILE__),
    chain_keeper_mutex_ ( __LINE__, __FILE__),     
    grouping_parameter_(1) {
    LOG_STACK_TRACE_POINT;
    
    // Do not modify details_ here as it is not yet initialized.
  }
  
  uint64_t Ping() 
  {
    LOG_STACK_TRACE_POINT;
    
    std::cout << "PING" << std::endl;
    
    return 1337;
  }

  NodeDetails Hello(uint64_t client, NodeDetails details) 
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard< fetch::mutex::Mutex > lock( client_details_mutex_ );
    client_details_[client] = details;
   //    SendConnectivityDetailsToGroups(details);    --- TODO Figure out why this dead locks
    return details_.details();
  }

  std::vector< NodeDetails > SuggestPeers() 
  {
    LOG_STACK_TRACE_POINT;
    
    if(need_more_connections() )
    {
      RequestPeerConnections( details_.details() );      
    }

    std::lock_guard< fetch::mutex::Mutex > lock( suggestion_mutex_ );
    
    return peers_with_few_followers_;
  }

  void RequestPeerConnections( NodeDetails details ) 
  {
    LOG_STACK_TRACE_POINT;
    
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
    LOG_STACK_TRACE_POINT;
    
    suggestion_mutex_.lock();
    bool found = false;
    auto it = peers_with_few_followers_.end();
    while( it != peers_with_few_followers_.begin() ) 
    {
      --it;
      if( (*it).public_key == details.public_key ) 
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


  void IncreaseGroupingParameter() 
  {
    LOG_STACK_TRACE_POINT;
    
    chain_keeper_mutex_.lock();
    uint32_t n = grouping_parameter_ << 1;

    fetch::logger.Debug("Increasing group parameter to ", n);
    
    struct IDClientPair 
    {
      std::size_t index;      
      client_shared_ptr_type client;      
    } ;
    
    std::map< std::size_t, std::vector< IDClientPair > > group_org;

    // Computing new grouping assignment
    for(std::size_t i=0; i < n; ++i)
    {
      group_org[i] = std::vector< IDClientPair >();      
    }
        
    for(std::size_t i=0; i < chain_keepers_.size(); ++i)
    {
      auto &details = chain_keepers_details_[i];
      auto &client = chain_keepers_[i];
      uint32_t s = details.group;
      group_org[s].push_back( {i, client} );
    }

    for(std::size_t i=0; i < grouping_parameter_; ++i )
    {
      std::vector< IDClientPair > &vec = group_org[i];
      if( vec.size() < 2 ) {
        TODO("Throw error - cannot perform grouping without more nodes");        
        chain_keeper_mutex_.unlock();
        return;        
      }

      std::size_t bucket2 = i + grouping_parameter_;
      std::vector< IDClientPair > &nvec = group_org[bucket2];

      std::size_t q = vec.size() >> 1; // Diving group into two groups
      for(; q != 0; --q)
      {
        nvec.push_back( vec.back() );
        vec.pop_back();        
      }      
    }

    // Assigning group values
    for(std::size_t i=0; i < n; ++i)
    {
      fetch::logger.Debug("Updating group nodes in group ", i);
      auto &vec = group_org[i];      
      for(auto &c: vec)
      {
        chain_keepers_details_[ c.index ].group = i;        
        c.client->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::SET_GROUP_NUMBER, uint32_t(i), uint32_t(n));

      }
    }
        
    grouping_parameter_ = n;    
    chain_keeper_mutex_.unlock();
  }

  uint32_t GetGroupingParameter()   
  {
    return grouping_parameter_;
  }
    
  
  ////////////////////////
  // Not service protocol
  client_shared_ptr_type ConnectChainKeeper(std::string const &host, uint16_t const &port ) 
  {
    LOG_STACK_TRACE_POINT;
    
    fetch::logger.Debug("Connecting to group ", host, ":", port);

    chain_keeper_mutex_.lock();
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_);
    chain_keeper_mutex_.unlock();
    
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Make variable

    EntryPoint ep = client->Call(fetch::protocols::FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::HELLO, host).As< EntryPoint >();    

    fetch::logger.Highlight("Before Add");
    
    with_node_details([](NodeDetails const &det) {
        for(auto &e: det.entry_points)
        {
          fetch::logger.Debug("  --- ", e.host, ":", e.port);

        }
        
      });    
    details_.AddEntryPoint( ep );    

    fetch::logger.Highlight("After Add");    
    with_node_details([](NodeDetails const &det) {
        for(auto &e: det.entry_points)
        {
          fetch::logger.Debug("  --- ", e.host, ":", e.port, " is group ", (e.configuration & EntryPoint::NODE_CHAIN_KEEPER));
        }
        
      });
    
    
    chain_keeper_mutex_.lock();
    chain_keepers_.push_back(client);
    chain_keepers_details_.push_back(ep);
    fetch::logger.Debug("Total group count = ", chain_keepers_.size());
    chain_keeper_mutex_.unlock();

    return client;    

  }  

  
  void SetClientIPCallback( std::function< std::string(uint64_t) > request_ip ) 
  {
    request_ip_ = request_ip;    
  }
  
  client_shared_ptr_type Connect( std::string const &host, uint16_t const &port ) 
  {
    LOG_STACK_TRACE_POINT;
    
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
          SwarmController::RequestPeerConnections(details);
        }) );
    
    client->Subscribe(protocol_, SwarmFeed::FEED_ENOUGH_CONNECTIONS ,
      new Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          SwarmController::EnoughPeerConnections(details);
        }) );    

    client->Subscribe(protocol_, SwarmFeed::FEED_ANNOUNCE_NEW_COMER ,
      new Function< void(NodeDetails) >([this](NodeDetails const& details) 
        {
          std::cout << "TODO: figure out what to do here" << std::endl;                          
        }) );    
    

    

    uint64_t ping = uint64_t(ping_promise);


    fetch::logger.Debug("Waiting for ping.");        
    if(ping == 1337)
    {
      fetch::logger.Info("Successfully got PONG");      

      peers_mutex_.lock(); 
      peers_.push_back( client ); 
      peers_mutex_.unlock();      
      
      service::Promise ip_promise = client->Call(protocol_, SwarmRPC::WHATS_MY_IP );
      std::string own_ip = ip_promise.As< std::string >();
      fetch::logger.Info("Node host is ", own_ip);

      // Creating 
      protocols::EntryPoint e;
      e.host = own_ip;
      e.group = 0;      
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
        e2.group = 0;
        e2.port = server_details.default_port;
        e2.http_port = server_details.default_http_port;
        e2.configuration = EntryPoint::NODE_SWARM;
        server_details.entry_points.push_back(e2);        
      }

//      SendConnectivityDetailsToGroups(server_details);
      peers_mutex_.lock();       
      server_details_[ client->handle() ] = server_details;
      peers_mutex_.unlock();       
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
    LOG_STACK_TRACE_POINT;
    
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
    chain_keeper_mutex_.lock();    
    fnc( chain_keepers_details_ );
   
    chain_keeper_mutex_.unlock();    
  }

  void with_shards_do(std::function< void(std::vector< client_shared_ptr_type > const &, std::vector< EntryPoint > &) > fnc) 
  {
    chain_keeper_mutex_.lock();    
    fnc( chain_keepers_, chain_keepers_details_ );
    chain_keeper_mutex_.unlock();    
  }

  void with_shards_do(std::function< void(std::vector< client_shared_ptr_type > const &) > fnc) 
  {
    chain_keeper_mutex_.lock();    
    fnc( chain_keepers_ );   
    chain_keeper_mutex_.unlock();    
  }
  
  
  
  void with_suggestions_do(std::function< void(std::vector< NodeDetails >  &) > fnc)  
  {
    std::lock_guard< fetch::mutex::Mutex > lock( suggestion_mutex_ );    
    fnc( peers_with_few_followers_ );    
  }

  void with_client_details_do(std::function< void(std::map< uint64_t, NodeDetails > const &) > fnc) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock( client_details_mutex_ );
    
    fnc( client_details_ );    
  }


  void with_peers_do( std::function< void(std::vector< client_shared_ptr_type >  ) > fnc ) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(peers_mutex_);
    
    fnc(peers_);    
  }

  void with_peers_do( std::function< void(std::vector< client_shared_ptr_type >, std::map< uint64_t, NodeDetails >&  ) > fnc ) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(peers_mutex_);
    
    fnc(peers_, server_details_);    
  }  

  void with_server_details_do(std::function< void(std::map< uint64_t, NodeDetails > const &) > fnc) 
  {
    peers_mutex_.lock();    
    fnc( server_details_ );
    peers_mutex_.unlock();    
  }

  void with_node_details(std::function< void(NodeDetails &) > fnc ) {
    details_.with_details( fnc );    
  }
private:

  void SendConnectivityDetailsToChainKeepers(NodeDetails const &server_details) 
  {
    for(auto const &e2: server_details.entry_points )
    {
      fetch::logger.Debug("Testing ", e2.host, ":", e2.port);
      
      if( e2.configuration & EntryPoint::NODE_CHAIN_KEEPER )
      {
        chain_keeper_mutex_.lock();
        fetch::logger.Debug(" - Group count = ", chain_keepers_.size() );
        for(std::size_t k=0; k < chain_keepers_.size(); ++k)
        {
          auto sd = chain_keepers_details_[k];
          fetch::logger.Debug(" - Connect ", e2.host, ":", e2.port,  " >> ", sd.host, ":", sd.port, "?");
          
          if(sd.group == e2.group )
          {
            std::cout << "       YES!"  <<std::endl;
            auto sc = chain_keepers_[k];
            sc->Call(FetchProtocols::CHAIN_KEEPER, ChainKeeperRPC::LISTEN_TO, e2);
          }           
        }

        chain_keeper_mutex_.unlock();
      }        
    }
  }
  
  uint64_t protocol_;
  network::ThreadManager *thread_manager_;  

  SharedNodeDetails &details_;

  std::map< uint64_t,  NodeDetails > client_details_;  
  mutable fetch::mutex::Mutex client_details_mutex_;
  
  std::vector< NodeDetails > peers_with_few_followers_;
  std::unordered_set< std::string > already_seen_;
  mutable fetch::mutex::Mutex suggestion_mutex_;
  

  std::function< std::string(uint64_t) > request_ip_;

  std::map< uint64_t, NodeDetails > server_details_;
  std::vector< client_shared_ptr_type > peers_;
  mutable fetch::mutex::Mutex peers_mutex_;

  std::vector< client_shared_ptr_type > chain_keepers_;
  std::vector< EntryPoint > chain_keepers_details_;
  mutable fetch::mutex::Mutex chain_keeper_mutex_;

  std::atomic< uint32_t > grouping_parameter_ ;


};


};
};
#endif 
