#ifndef NODE_FUNCTIONALITY
#define NODE_FUNCTIONALITY

#include"byte_array/referenced_byte_array.hpp"
#include"byte_array/const_byte_array.hpp"
#include"serializer/referenced_byte_array.hpp"

#include"chain/transaction.hpp"
#include"transaction_serializer.hpp"


#include"rpc/publication_feed.hpp"
#include"mutex.hpp"
#include"commands.hpp"

#include<map>
#include<vector>



class NodeChainManager : public fetch::rpc::HasPublicationFeed {  
public:
  typedef fetch::byte_array::ConstByteArray transaction_body_type;
  typedef fetch::chain::BasicTransaction< transaction_body_type > transaction_type;
  typedef typename transaction_type::digest_type digest_type;

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

  

  /*
  void PushBlock( block_type const &block) {

  }

  std::vector< block_type > GetBlocks( std::size_t const &from) {

  }
  */  
private:
  fetch::mutex::Mutex tx_mutex_;
  std::vector< digest_type > incoming_;
  std::map< digest_type, transaction_type > known_transactions_;

  //  std::vector< block_type > chain_;
};

#endif 
