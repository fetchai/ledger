#ifndef NODE_FUNCTIONALITY
#define NODE_FUNCTIONALITY

#include"byte_array/referenced_byte_array.hpp"
#include"byte_array/const_byte_array.hpp"
#include"serializer/referenced_byte_array.hpp"

#include"chain/transaction.hpp"
#include"chain/block.hpp"
#include"chain/consensus/proof_of_work.hpp"
#include"transaction_serializer.hpp"


#include"rpc/publication_feed.hpp"
#include"mutex.hpp"
#include"commands.hpp"

#include<map>
#include<vector>

struct BlockBody {
  fetch::byte_array::ByteArray previous_hash;  
  fetch::byte_array::ByteArray transaction_hash;
};

template< typename T >
T& Serialize( T & serializer, BlockBody const &body) {
  serializer << body.previous_hash << body.transaction_hash;
  return serializer;
}

template< typename T >
T& Deserialize( T & serializer, BlockBody const &body) {
  serializer >> body.previous_hash >> body.transaction_hash;
  return serializer;  
}


class NodeChainManager : public fetch::rpc::HasPublicationFeed {  
public:
  // Transaction defs
  typedef fetch::byte_array::ConstByteArray transaction_body_type;
  typedef fetch::chain::BasicTransaction< transaction_body_type > transaction_type;
  typedef typename transaction_type::digest_type tx_digest_type;

  // Block defs  
  typedef fetch::chain::consensus::ProofOfWork proof_type;
  typedef BlockBody block_body_type;
  typedef typename proof_type::header_type block_header_type;
  typedef fetch::chain::BasicBlock< block_body_type, proof_type, fetch::crypto::SHA256 > block_type;  

  NodeChainManager() {
    block_body_type genesis_body;
    block_type genesis_block;

    genesis_body.previous_hash = "genesis";
    genesis_body.transaction_hash = "genesis";
    genesis_block.SetBody( genesis_body );
    chain_.push_back( genesis_block );
  }
  
  /* Remote control and internal functionality to push new transactions
   * @tx is the transaction that will be added.
   *
   * This function adds a new transaction to the queue of unmined
   * transactions.
   *
   * @return true if the transaction was add and false otherwise.
   */
  bool PushTransaction( transaction_type const &tx ) {    
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

    this->Publish(PeerToPeerCommands::BROADCAST_TRANSACTION, tx );
    return true;
  }


  block_type GetNextBlock() {
    block_body_type body;
    block_type block;

    block_mutex_.lock();
    body.previous_hash = chain_.front().header();
    block_mutex_.unlock();
    
    tx_mutex_.lock();    
    body.transaction_hash =  incoming_.front();
    tx_mutex_.unlock();
    
    block.SetBody( body );
    return block;
  }
  
  void PushBlock( block_type const &block) {
    block_mutex_.lock();

    assert( known_blocks_.find( block.header() ) == known_blocks_.end() );
    known_blocks_[block.header()] = block;
        
    // TODO: Compute accumulated work and find best chain.
    
    block_mutex_.unlock();

    // TODO: Trim blocks away that are more than
    //    this->Publish(PeerToPeerCommands::BROADCAST_BLOCK, block );    
  }

  void Commit() {
    // Always do a delayed commit to avoid resyncing as much as possible.
  }
  
  /*
  std::vector< block_type > GetBlocks( std::size_t const &from) {

  }
  */  
private:
  fetch::mutex::Mutex tx_mutex_;
  std::vector< tx_digest_type > incoming_;
  std::map< tx_digest_type, transaction_type > known_transactions_;


  fetch::mutex::Mutex block_mutex_;
  std::map< block_header_type, block_type > known_blocks_;
  std::vector< block_type > chain_;
};

#endif 
