#ifndef PROTOCOLS_SHARD_MANAGER_HPP
#define PROTOCOLS_SHARD_MANAGER_HPP
#include"byte_array/referenced_byte_array.hpp"
#include"byte_array/const_byte_array.hpp"
#include"serializer/referenced_byte_array.hpp"

#include"protocols/fetch_protocols.hpp"
#include"chain/transaction.hpp"
#include"chain/block.hpp"
#include"chain/consensus/proof_of_work.hpp"
#include"protocols/shard/block.hpp"

#include "service/client.hpp"
#include"service/publication_feed.hpp"
#include"mutex.hpp"
#include"protocols/shard/commands.hpp"

#include"protocols/swarm/entry_point.hpp"
#include"logger.hpp"

#include<map>
#include<vector>
#include<limits>
#include<stack>


namespace fetch
{
namespace protocols 
{


class ShardManager : public fetch::service::HasPublicationFeed 
{
public:


  // Transaction defs
  typedef fetch::chain::Transaction transaction_type;
  typedef typename transaction_type::digest_type tx_digest_type;

  // Block defs  
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef BlockMetaData block_meta_data_type;
  typedef fetch::chain::BasicBlock< block_body_type, proof_type, fetch::crypto::SHA256, block_meta_data_type > block_type;  

  // Other shards  
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;

  struct PartialChain 
  {
    block_header_type start;    
    block_header_type end;
    block_header_type next_missing;    
  };  

  struct ConnectionDetails
  {
    std::string host;
    uint16_t port;
    
    bool operator==(ConnectionDetails const &other) const 
    {
      return (host == other.host) && (port == other.port);      
    }

    bool operator<(ConnectionDetails const &other) const 
    {
      if(port == other.port)
        return host < other.host;
      return port < other.port;      
    }    
  };
  std::map<ConnectionDetails, int > last_connection_attempt_;
  fetch::mutex::Mutex connection_mutex_;
  
    

  
  ShardManager(uint64_t const& protocol,
    network::ThreadManager *thread_manager,
    EntryPoint& details) :
    thread_manager_(thread_manager),
    details_(details),
    block_mutex_( __LINE__, __FILE__),
    sharding_parameter_(1)
  {    
    details_.configuration = EntryPoint::NODE_SHARD;
    
    block_body_type genesis_body;
    block_type genesis_block;

    genesis_body.previous_hash = "genesis";
    genesis_body.transaction_hash = "genesis";
    genesis_block.SetBody( genesis_body );

    genesis_block.meta_data().total_work = 0;
    genesis_block.meta_data().block_number = 0;

    ResetNextHead();
    
    PushBlock( genesis_block );    
  }

  // TODO: Change signature to std::vector< EntryPoint >
  EntryPoint Hello(std::string host) 
  {
    details_.configuration = EntryPoint::NODE_SHARD;
    if(details_.host != host ) { 
      details_.host = host;
    }
    
    return details_;
  }
  
  block_type ExchangeHeads(block_type head_candidate) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(block_mutex_);
    fetch::logger.Debug("Sending head");
    
    // TODO: Check which head is better
    
    return head_;    
  }

  std::vector< block_type > RequestBlocksFrom(block_header_type next_hash, uint16_t preferred_block_count) 
  {
    std::vector< block_type > ret;

    if( preferred_block_count > 10 ) preferred_block_count = 10;    
    ret.reserve( preferred_block_count );
    
    std::lock_guard< fetch::mutex::Mutex > lock(block_mutex_);
    std::size_t i =0;    
    while( (i< preferred_block_count) && (chains_.find( next_hash ) !=chains_.end() ) ) {
      auto const &block = chains_[next_hash];
      ret.push_back( block );

      next_hash = block.body().previous_hash;
      ++i;
    }    

    return ret;    
  }
  

  
  bool PushTransaction( transaction_type tx ) {
    tx_mutex_.lock();

    tx.UpdateDigest();
    
    if(known_transactions_.find( tx.digest()  ) != known_transactions_.end() ) {      
      tx_mutex_.unlock();
      return false;
    }
    
    known_transactions_[ tx.digest() ] = tx;
    bool belongs_to_shard = true;
    uint32_t shard = details_.shard;

    TODO("Implement shard checking");    
    
    tx_mutex_.unlock();

    if(!belongs_to_shard)
    {
      fetch::logger.Info("Transaction does not belong to this shard", shard);
      
    }
    
    
    TODO("Verify transaction");
    
    tx_mutex_.lock();
    incoming_.push_back( tx.digest() );
    tx_mutex_.unlock();

    this->Publish(ShardFeed::FEED_BROADCAST_TRANSACTION, tx );
    return true;
  }


  block_type GetNextBlock() {
    block_body_type body;
    block_type block;

    block_mutex_.lock();
    body.previous_hash = head_.header();
    block_mutex_.unlock();
    
    tx_mutex_.lock();
    if(incoming_.size() == 0) {
      body.transaction_hash =  "";
    } else {
      body.transaction_hash =  incoming_.front();
    }    
    tx_mutex_.unlock();
    
    block.SetBody( body );
    return block;
  }
  
  void PushBlock(block_type block) {
    block_mutex_.lock();
    std::cout << "Pushing block" << std::endl;
    
    // Only record blocks that are new
    if( chains_.find( block.header() ) != chains_.end() ) {
      std::cout << "Nothing to do for block" << std::endl;
      
      block_mutex_.unlock();
      std::cout << "EXIT Pushing block 1" << std::endl;
      return ;
    }
    
    assert( chains_.find( block.header() ) == chains_.end() );
    block_header_type header = block.header();
    block.meta_data().loose_chain = true;    
    chains_[block.header()] = block;
    

    // Check if block is adding to a loose chain.
    bool was_loose = false;    
    if(loose_chain_tops_.find( block.body().previous_hash ) != loose_chain_tops_.end() )
    {
      /*                                 
       * Main chain
       * with path to genesis             
       * ┌──────┐                        
       * │      │                        
       * │      │   Missing block        
       * └──────┘   ┌ ─ ─ ─              
       *     │             │             
       *     │      │                    
       *     ▼       ─ ─ ─ ┘             
       * ┌──────┐       │                
       * │      │       │                
       * │      │       ▼ Loose chains   
       * └──────┘   ┌──────┐ with no path            
       *     │      │      │ to genesis
       *     │      │      │             
       *     ▼      └──────┘             
       * ┌──────┐       │                
       * │      │       └──┐             
       */
      was_loose = true;
      
      std::size_t i = loose_chain_tops_[block.body().previous_hash];
      auto it = loose_chain_tops_.find( block.body().previous_hash ) ;
      loose_chain_tops_.erase( it );

      assert( i < loose_chains_.size() );      
      auto &pc = loose_chains_[i];

      pc.start = header;
      loose_chain_tops_[header] = i;      
    }


    if(loose_chain_bottoms_.find( header ) != loose_chain_bottoms_.end() )
    {
      /*         
       * Chains with
       * path to         
       * genesis         Loose chains        
       * ┌──────┐       │       │      │      
       * │      │       │       │      │      
       * │      │       ▼       └──────┘      
       * └──────┘   ┌──────┐        │         
       *     │      │      │        ▼         
       *     │      │      │    ┌──────┐      
       *     ▼      └──────┘    │      │      
       * ┌──────┐       │       │      │      
       * │      │       └──┐    └──────┘      
       * │      │          │        │         
       * └──────┘          ▼        │         
       *     │         ┌ ─ ─ ─      │         
       *     │                │     │         
       *     ▼         │       ◀────┘         
       * ┌──────┐       ─ ─ ─ ┘               
       * │      │                            
       * │      │      Missing block  
       * └──────┘                             
       */
      was_loose = true;
      std::vector< uint64_t > lchains = loose_chain_bottoms_[header];
      auto it = loose_chain_bottoms_.find( header ) ;
      loose_chain_bottoms_.erase( it );

      for(auto &id: lchains) {
        auto &pc = loose_chains_[id];
        pc.end = header;
        pc.next_missing = block.body().previous_hash;        
      }

      if( loose_chain_bottoms_.find( block.body().previous_hash ) == loose_chain_bottoms_.end() )
      {
        // Even though the chains merge, the remain many separate chains
        loose_chain_bottoms_[block.body().previous_hash] = lchains;
      }
      else
      {
        
        auto &l = loose_chain_bottoms_[block.body().previous_hash];
        for(auto &id : lchains)
        {
          l.push_back(id);          
        }        
      }
            

      /* Chain with 
       * path 
       * to genesis    Loose chains
       *
       * ┌──────┐       │       │      │      
       * │      │       └──┐    └──────┘      
       * │      │          │        │         
       * └──────┘          ▼        │         
       *     │         ┌ ─ ─ ─      │         
       *     │                │     │         
       *     ▼         │       ◀────┘         
       * ┌──────┐       ─ ─ ─ ┘               
       * │      │          │                  
       * │      │◀─────────┘   Missing block  
       * └──────┘                             
       *     │
       *
       * Checking if path to genesis block exists
       */
      if( chains_.find(block.body().previous_hash) != chains_.end() ) {
        auto &next = chains_[block.body().previous_hash];

        // If so, the block is final and we are ready to move the loose chain to the
        // main tree
        if(next.meta_data().loose_chain == false) {
          auto &l = loose_chain_bottoms_[block.body().previous_hash];
          for(auto &id: l)
          {
            auto &pc = loose_chains_[id];
            block_header_type h = pc.start;    
            block_type b = chains_[header];

            block_mutex_.unlock();
            AttachBlock(h,b);
            block_mutex_.lock();
            
            auto tit = loose_chain_tops_.find( h );            
            if(tit != loose_chain_tops_.end())
            {
              loose_chain_tops_.erase( tit );              
            }
            
            loose_chains_.erase( loose_chains_.find( id ) );            
          }
          

          auto it = loose_chain_bottoms_.find(block.body().previous_hash);
          loose_chain_bottoms_.erase(it);          
        }
      }
    }

    block_mutex_.unlock();
    std::cout << "Publishing block 1" << std::endl;    
    this->Publish(ShardFeed::FEED_BROADCAST_BLOCK, block );

    std::cout << "Publishing block 2" << std::endl;          
    shard_friends_mutex_.lock();
    for(auto &c: shard_friends_)
    {
      c->Call(FetchProtocols::SHARD,  ShardRPC::PUSH_BLOCK, block );      
    }
    shard_friends_mutex_.unlock();

    
    if(was_loose)
    {
      std::cout << "EXITING block 2" << std::endl;  
      return; 
    }

    std::cout << "Attachign block" << std::endl;              
    // FInally we attach the block if it does not belong to a loose chain
    AttachBlock(header, block);
      std::cout << "EXITING block 3" << std::endl;      
  }

  
  void Commit() {    
    block_mutex_.lock();
    // We only commit if there actually is a new block
    if( next_head_.meta_data().block_number > 0 ) {
     
      head_ = next_head_;
      std::cout << "Applying block: " << head_.meta_data().block_number << " " <<  head_.meta_data().total_work <<  std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64( head_.body().previous_hash ) << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64( head_.header() ) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64( head_.body().transaction_hash ) << ")" << std::endl;

      // TODO: Update transaction order
      
      // Removing TX from queue
      std::size_t deltx = -1;      
      for(std::size_t i=0; i < incoming_.size(); ++i)
      {
        if(head_.body().transaction_hash == incoming_[i])
        {
          deltx = i;
          break;          
        }        
      }

      if(deltx != std::size_t( -1 ) )
      {
        incoming_.erase( incoming_.begin() + deltx);        
      }
      
      
      ResetNextHead();
    }
    block_mutex_.unlock();    
  }
  

  void ConnectTo(std::string const &host, uint16_t const &port ) 
  {
    /*
      TODO: Make a connection directory
    connection_mutex_.lock();
    ConnectionDetails cc = {host, port};

    bool should_connect = false;
    
    if(last_connection_attempt_.find(cc) == last_connection_attempt_.end()) {
      should_connect = true;      
      last_connection_attempt_[cc] = 1; // TODO: Make time stamp.       
    }
    
    connection_mutex_.unlock();
    if(!should_connect) return;
    */

    // Chained tasks
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_);

    EntryPoint d;
    d.host = host;    
    d.port = port;
    d.http_port = -1; 
    d.shard = 0; // TODO: get and check that it is right
    d.configuration = 0;  


    block_mutex_.lock();    
    block_type head_copy = head_;    
    block_mutex_.unlock();
    
    auto promise1 = client->Call(FetchProtocols::SHARD, ShardRPC::EXCHANGE_HEADS, head_copy);
   
    client->Subscribe(FetchProtocols::SHARD,  ShardFeed::FEED_BROADCAST_BLOCK,
      new service::Function< void(block_type) >([this]( block_type const& block) 
        {
          this->PushBlock(block);          
        }));             
    
    try {

      fetch::logger.Debug("Deserializing head");
      promise1.Wait();

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
      
      comp_head.meta_data() = block_meta_data_type();      
      
      PushBlock(comp_head);

      // Adding friend if everything goes well.
      shard_friends_mutex_.lock();
      shard_friends_.push_back(client);
      friends_details_.push_back(d);
      shard_friends_mutex_.unlock();
    } catch( ... ) {

      fetch::logger.Error("Failed to retrieve head - possibly protocol error.");
    }
    
     
    /*

    auto task4 = [&](service::Promise &promise1) {
      block_type comp_head = promise1.As< block_type >();
      comp_head.meta_data() = block_meta_data_type();      
      
      PushBlock(comp_head);
      fetch::logger.Debug("Done");
     
    };
    
    auto task3 = [&](service::Promise &promise1) {
      shard_friends_mutex_.lock();
      client->Subscribe(FetchProtocols::SHARD,  ShardFeed::FEED_BROADCAST_BLOCK,
        new service::Function< void(block_type) >([this]( block_type const& block) 
          {
            this->PushBlock(block);          
          }));             
      shard_friends_mutex_.unlock();

      thread_manager_->io_service().post([&]() {
          fetch::logger.Debug("Issueing task 4"); 
          task4(promise1);
      });      
    };
    
    auto task2 = [&]() {
      block_mutex_.lock(); 
      auto promise1 = client->Call(FetchProtocols::SHARD, ShardRPC::EXCHANGE_HEADS, head_);
      block_mutex_.unlock();

      thread_manager_->io_service().post([&]() {
          fetch::logger.Debug("Issueing task 3");
          task3(promise1);
      });
    };
    
    auto task1 = [&]() {

      

      EntryPoint d;
      d.host = host;    
      d.port = port;
      d.http_port = -1; 
      d.shard = 0; // TODO: get and check that it is right
      d.configuration = 0;  


      fetch::logger.Debug("Just before crash");
      shard_friends_mutex_.lock();
      fetch::logger.Debug("Just after crash");      
      shard_friends_.push_back(client);
      friends_details_.push_back(d);
      shard_friends_mutex_.unlock();
      
      thread_manager_->io_service().post([&]() {
          fetch::logger.Debug("Issueing task 2");          
          task2();
      });
      
    } ;
    
    
    thread_manager_->io_service().post([&]() {        
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Make variable
          fetch::logger.Debug("Issueing task 1");
        task1();
      });
    */
  }

  void ListenTo(EntryPoint e) 
  {
    fetch::logger.Debug( "Thinking of connecting to ", e.host,  ":", e.port );
    
    bool found = false;
    shard_friends_mutex_.lock();    
    for(auto &d: friends_details_)
    {
      if((d.host == e.host) &&
        (d.port == e.port )  ) {
        found = true;
        break;
      }
    }

    shard_friends_mutex_.unlock();
    if(!found)
    {
      ConnectTo(e.host, e.port); 
    }
    
  }

  void SetShardNumber(uint32_t shard, uint32_t total_shards) 
  {
    fetch::logger.Debug("Setting shard numbers: ", shard, " ", total_shards);    
    sharding_parameter_ = total_shards;
    details_.shard = shard;    
  }
  
  uint32_t count_outgoing_connections() 
  {
    std::cout << "Was here??" << std::endl;    
    std::lock_guard< fetch::mutex::Mutex > lock( shard_friends_mutex_ );
    std::cout << "Was here???" << std::endl;    
    return shard_friends_.size();    
  }

  uint32_t shard_number() 
  {
    return details_.shard;    
  }
  

  void with_peers_do( std::function< void( std::vector< client_shared_ptr_type > , std::vector< EntryPoint > const& ) > fnc ) 
  {
    shard_friends_mutex_.lock();
    fnc( shard_friends_, friends_details_ );    
    shard_friends_mutex_.unlock();
  }

  void with_blocks_do( std::function< void(block_type, std::map< block_header_type, block_type >)  > fnc ) 
  {
    block_mutex_.lock();
    fnc( head_, chains_ );    
    block_mutex_.unlock();
  }

  void with_transactions_do( std::function< void(std::vector< tx_digest_type >,  std::map< tx_digest_type, transaction_type >) > fnc )
  {
    tx_mutex_.lock();
    fnc( incoming_, known_transactions_ );    
    tx_mutex_.unlock();
  }

  void with_loose_chains_do( std::function< void( std::map< uint64_t,  PartialChain > ) > fnc ) 
  {
    block_mutex_.lock();
    fnc( loose_chains_ );    
    block_mutex_.unlock();
  }
  
  
private:
  void AttachBlock(block_header_type &header, block_type &block) 
  {
    block_mutex_.lock();
    std::cout << "Attaching block!" << std::endl;
    
    // Tracing the way back to a chain that leads to genesis
    // TODO, FIXME: Suceptible to attack: Place a block that creates a loop.
    std::stack< block_header_type > visited_blocks;    
    while(chains_.find(header) != chains_.end()) {      
      auto b = chains_[header];
      visited_blocks.push( header );

      /*
        TODO: We can do performance optimisation here.
        This is wrong as the code does not support half chains at the moment
      if(block.meta_data().block_number != block_meta_data_type::UNDEFINED)
      {        
        break;
      }
      */
      
      header = b.body().previous_hash;
    }

    // By design visited blocks must contain the latest submitted block.
    assert( visited_blocks.size() > 0 );
    block_header_type earliest_header = visited_blocks.top();
    block_type earliest_block = chains_[earliest_header];
    
    
    // Computing the total work that went into the chain.
    if(block.body().transaction_hash == "genesis") {
      std::cout << "Adding genesis" << std::endl;
      block.meta_data().loose_chain = false;      
      chains_[block.header()] = block;
      
      head_ = block;
      block_mutex_.unlock();
      return;
    } else if( earliest_block.body().transaction_hash != "genesis" ) {
      // Creating loose chain - we are sure that it does not add to existing
      // loose chains beause we checked that earlier.

      PartialChain pc;
      pc.start = block.header();
      pc.end = earliest_block.header();      
      pc.next_missing = earliest_block.body().previous_hash;

      
      std::size_t i = loose_chain_counter_;
      loose_chains_[i] = pc;      
      loose_chain_tops_[ pc.start] = i;
      ++loose_chain_counter_;

      
      if(loose_chain_bottoms_.find(pc.next_missing) == loose_chain_bottoms_.end() ) {
        loose_chain_bottoms_[ pc.next_missing ] = std::vector< uint64_t >();       
      }
      
      loose_chain_bottoms_[ pc.next_missing ].push_back(i);      
           
    } else {
      std::cout << "Found root: " << header << std::endl;
      block.meta_data().loose_chain = false;      
      chains_[block.header()] = block;
      
      header = visited_blocks.top();
      auto b1 = chains_[header];
      visited_blocks.pop();

      while(!visited_blocks.empty()) {
        header = visited_blocks.top();
        auto b2 = chains_[header];

        auto &p = b2.proof();
        p();
        double work = fetch::math::Log( p.digest() );
        
        b2.meta_data() = b1.meta_data();
        ++b2.meta_data().block_number;

        // TODO: Check the correct way to compute the strongest chain - looks wrong
        b2.meta_data().total_work += work;
        b2.meta_data().loose_chain = false;        
        chains_[header] = b2;

        b1 = b2;
        visited_blocks.pop();

      }

      block = chains_[header];
    }

    if(block.meta_data().total_work > next_head_.meta_data().total_work) {
      next_head_ = block;
      block_mutex_.unlock();
      this->Commit();       
    } else {
      block_mutex_.unlock();   
    }
  }
  
  void ResetNextHead() {
    next_head_.meta_data().total_work = 0;
    next_head_.meta_data().block_number = 0;    
  }

  network::ThreadManager *thread_manager_;    
  EntryPoint &details_;  
  
  fetch::mutex::Mutex tx_mutex_;
  std::vector< tx_digest_type > incoming_;
  std::map< tx_digest_type, transaction_type > known_transactions_;
  std::vector< transaction_type > tx_order_;


  fetch::mutex::Mutex block_mutex_;
  std::map< block_header_type, block_type > chains_;

  std::map< uint64_t, PartialChain > loose_chains_;
  uint64_t loose_chain_counter_ = 0;  
  std::map< block_header_type, std::vector< uint64_t > > loose_chain_bottoms_;
  std::map< block_header_type, uint64_t > loose_chain_tops_;  
  
  std::vector< block_header_type > heads_;  
  block_type head_, next_head_;


  std::vector< client_shared_ptr_type > shard_friends_;
  std::vector< EntryPoint > friends_details_;  
  fetch::mutex::Mutex shard_friends_mutex_;  

  std::atomic< uint32_t > sharding_parameter_ ;
};


};
};

#endif
