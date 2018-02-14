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
  typedef fetch::byte_array::ConstByteArray transaction_body_type;
  typedef fetch::chain::BasicTransaction< transaction_body_type > transaction_type;
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

  ShardManager(uint64_t const& protocol,
    network::ThreadManager *thread_manager,
    EntryPoint& details) :
    thread_manager_(thread_manager),
    details_(details)    
  {    
    shard_parameter_ = 0;

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
   
    if(details_.host != host ) {      
      details_.host = host;
    }
    
    return details_;
  }
  
  block_type ExchangeHeads(block_type head_candidate) 
  {
    std::lock_guard< fetch::mutex::Mutex > lock(block_mutex_);
    
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
    if(known_transactions_.find( tx.digest()  ) != known_transactions_.end() ) {      
      tx_mutex_.unlock();
      return false;
    }
    
    known_transactions_[ tx.digest() ] = tx;
    tx_mutex_.unlock();

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

    if( chains_.find( block.header() ) != chains_.end() ) {
      /*
      std::cout << "Already known block: " << fetch::byte_array::ToBase64(block.header() ) << std::endl;
      auto b2 = chains_[block.header()];
      std::cout << "Block 1: " << std::endl;
      std::cout << " - " << fetch::byte_array::ToBase64( block.body().previous_hash )  << std::endl;
      std::cout << " - " << fetch::byte_array::ToBase64( block.body().transaction_hash )  << std::endl;
      std::cout << "Block 2: " << std::endl;
      std::cout << " - " << fetch::byte_array::ToBase64( b2.body().previous_hash )  << std::endl;
      std::cout << " - " << fetch::byte_array::ToBase64( b2.body().transaction_hash )  << std::endl;
      std::cout << " - " << b2.meta_data().block_number  << std::endl;      
      */
      block_mutex_.unlock();      
      return ;
    }
    assert( chains_.find( block.header() ) == chains_.end() );    
    chains_[block.header()] = block;
    
    block_header_type header = block.header();
    std::stack< block_header_type > visited_blocks;

    // TODO: Check if block is adding to a loose chain.
    

    // Tracing the way back to a chain that leads to genesis
    // TODO, FIXME: Suceptible to attack: Place a block that creates a loop.
    while(chains_.find(header) != chains_.end()) {      
      auto b = chains_[header];
      visited_blocks.push( header );
      
      if(block.meta_data().block_number != block_meta_data_type::UNDEFINED)
      {
        break;
      }
      
      header = b.body().previous_hash;
    }

    // By design visited blocks must contain the latest submitted block.
    assert( visited_blocks.size() > 0 );
    block_header_type earliest_header = visited_blocks.top();
    block_type earliest_block = chains_[earliest_header];
    
    
    // Computing the total work that went into the chain.
    if(block.body().transaction_hash == "genesis") {
      std::cout << "Adding genesis" << std::endl;
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
//      if(loose_block_register_.find( ) ) 
      

      // TODO: add 
      std::cout << "TODO: NEED TO SYNC " << std::endl;
      
    } else {
      std::cout << "Found root: " << header << std::endl;
      
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
        chains_[header] = b2;

        b1 = b2;
        visited_blocks.pop();
      }

      block = chains_[header];
    }

    if(block.meta_data().total_work > next_head_.meta_data().total_work) {
      // TODO: Change branch if not succeeding blocks
      next_head_ = block;
    }
    
    block_mutex_.unlock();    

    // TODO: Trim blocks away that are more than
//    this->Publish(PeerToPeerCommands::BROADCAST_BLOCK, block );    
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
    client_shared_ptr_type client = std::make_shared< client_type >(host, port, thread_manager_);
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ); // TODO: Make variable
    shard_friends_mutex_.lock();
    EntryPoint d;
    d.host = host;    
    d.port = port;
    d.http_port = -1; 
    d.shard = 0; // TODO: get and check that it is right
    d.configuration = 0;  
    shard_friends_.push_back(client);
    friends_details_.push_back(d);


    block_mutex_.lock();    
    auto promise1 = client->Call(FetchProtocols::SHARD, ShardRPC::EXCHANGE_HEADS, head_);    
    block_mutex_.unlock();

    block_type comp_head = promise1.As< block_type >();
    comp_head.meta_data() = block_meta_data_type();
    PushBlock(comp_head);       

    
    shard_friends_mutex_.unlock();
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
  
  
private:
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

  struct PartialChain 
  {
    block_header_type start;    
    block_header_type end;
    block_header_type next_missing;    
  };
  

  std::vector< PartialChain > loose_chains_;  
  std::map< block_header_type, uint64_t > loose_chain_bottoms_;
  std::map< block_header_type, uint64_t > loose_chain_tops_;  
  
  std::vector< block_header_type > heads_;  
  block_type head_, next_head_;


  std::vector< client_shared_ptr_type > shard_friends_;
  std::vector< EntryPoint > friends_details_;  
  fetch::mutex::Mutex shard_friends_mutex_;  

  std::atomic< uint16_t > shard_parameter_ ;  
};


};
};

#endif
