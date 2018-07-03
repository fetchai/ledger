#include<iostream>
#include"storage/document_store.hpp"
#include"storage/document_store_protocol.hpp"
#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"storage/object_store_syncronisation_protocol.hpp"
#include"network/service/server.hpp"
#include"core/commandline/cli_header.hpp"
#include"ledger/chain/transaction.hpp"
using namespace fetch;
using namespace fetch::storage;

class LaneService : public fetch::service::ServiceServer< fetch::network::TCPServer > {
public:
  LaneService(uint16_t port, fetch::network::ThreadManager tm) : ServiceServer(port, tm) {
    store_ = new RevertibleDocumentStore();
    store_->Load("a.db", "b.db", "c.db", "d.db", true);    
    store_protocol_ = new RevertibleDocumentStoreProtocol(store_);    
    this->Add(0, store_protocol_ );

    tx_store_ = new ObjectStore< fetch::chain::Transaction >();
    tx_store_->Load("e.db", "f.db", true);    
    tx_store_protocol_ = new ObjectStoreProtocol< fetch::chain::Transaction >(tx_store_);    
    this->Add(1, tx_store_protocol_ );

    p2p_sync_protocol_ = new ObjectStoreSyncronisationProtocol< fetch::chain::Transaction >(tx_store_ );
  }
  
  ~LaneService() 
  {
    // TODO: Remove protocol
    delete store_protocol_;
    delete store_;

    // TODO: Remove protocol
    delete tx_store_protocol_;
    delete tx_store_;

    delete p2p_sync_protocol_;
    
  }
  
private:
  RevertibleDocumentStore *store_;
  RevertibleDocumentStoreProtocol *store_protocol_;

  ObjectStore<fetch::chain::Transaction> *tx_store_;
  ObjectStoreProtocol<fetch::chain::Transaction> *tx_store_protocol_;

  ObjectStoreSyncronisationProtocol< fetch::chain::Transaction > *p2p_sync_protocol_;
//  ObjectStoreP2PController< fetch::chain::Transaction > *p2p_controller_;  
};


int main() 
{
  fetch::logger.DisableLogger();
  
  fetch::network::ThreadManager tm(8);  
  LaneService serv(8080, tm);
  tm.Start();

  std::string dummy;
  fetch::commandline::DisplayCLIHeader("Single lane server");
  
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  tm.Stop();


  return 0;
}
