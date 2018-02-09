#ifndef PROTOCOLS_SHARD_MANAGER_HPP
#define PROTOCOLS_SHARD_MANAGER_HPP
#include"byte_array/referenced_byte_array.hpp"
#include"byte_array/const_byte_array.hpp"
#include"serializer/referenced_byte_array.hpp"


#include"chain/transaction.hpp"
#include"chain/block.hpp"
#include"chain/consensus/proof_of_work.hpp"
#include"protocols/shard/block.hpp"

#include "service/client.hpp"
#include"service/publication_feed.hpp"
#include"mutex.hpp"
#include"protocols/shard/commands.hpp"




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

  
  typedef fetch::service::ServiceClient< fetch::network::TCPClient > client_type;
  typedef std::shared_ptr< client_type >  client_shared_ptr_type;

  ShardManager() {

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
    body.transaction_hash =  incoming_.front();
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


    // Tracing the way back to a chain that leads to genesis
    while(chains_.find(header) != chains_.end()) {      
      auto b = chains_[header];
      visited_blocks.push( header );
      
      if(block.meta_data().block_number != block_meta_data_type::UNDEFINED) {
        break;
      }
      
      header = b.body().previous_hash;
    }

    // Computing the total work that went into the chain.
    if(block.body().transaction_hash == "genesis") {
      std::cout << "Adding genesis" << std::endl;
      head_ = block;
      block_mutex_.unlock();
      return;
    } else if(chains_.find(header) != chains_.end()) {

      // TODO: Remove all of visited blocks from loose_blocks_
      // Add block.header to loose_blocks
      TODO_FAIL("not implemented yet");
    } else {
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
      // TODO: Remove transactions from queue
      head_ = next_head_;
      std::cout << "Applying block: " << head_.meta_data().block_number << " " <<  head_.meta_data().total_work <<  std::endl;
      std::cout << "  <- " << fetch::byte_array::ToBase64( head_.body().previous_hash ) << std::endl;
      std::cout << "   = " << fetch::byte_array::ToBase64( head_.header() ) << std::endl;
      std::cout << "    (" << fetch::byte_array::ToBase64( head_.body().transaction_hash ) << ")" << std::endl;
      ResetNextHead();
    }
    block_mutex_.unlock();    
  }
  
  /*
  std::vector< block_type > GetBlocks( std::size_t const &from) {

  }
  */  
private:
  void ResetNextHead() {
    next_head_.meta_data().total_work = 0;
    next_head_.meta_data().block_number = 0;    
  }
  
  fetch::mutex::Mutex tx_mutex_;
  std::vector< tx_digest_type > incoming_;
  std::map< tx_digest_type, transaction_type > known_transactions_;


  fetch::mutex::Mutex block_mutex_;
  std::map< block_header_type, block_type > chains_;
  std::vector< block_header_type > loose_blocks_;  
  std::vector< block_header_type > heads_;  
  block_type head_, next_head_;
};


};
};

#endif
