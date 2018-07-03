#include<iostream>
#include"storage/document_store.hpp"
#include"storage/document_store_protocol.hpp"
#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"storage/object_store_syncronisation_protocol.hpp"
#include"network/service/server.hpp"
#include"core/commandline/cli_header.hpp"
#include"core/commandline/parameter_parser.hpp"
#include"ledger/chain/transaction.hpp"

#include<sstream>
using namespace fetch;
using namespace fetch::storage;

class LaneService : public fetch::service::ServiceServer< fetch::network::TCPServer > {
public:
  LaneService(uint32_t const &lane, uint16_t port, fetch::network::ThreadManager tm) : ServiceServer(port, tm) {
    store_ = new RevertibleDocumentStore();
    std::stringstream s;
    s << "lane" << lane << "_";
    std::string prefix = s.str();    
    
    store_->Load(prefix+"a.db", prefix+"b.db", prefix+"c.db", prefix+"d.db", true);    
    store_protocol_ = new RevertibleDocumentStoreProtocol(store_);
    store_protocol_->AddMiddleware([lane](uint32_t const &n, byte_array::ConstByteArray const &msg) {
        std::cout << "Getting request on lane " << lane << " from client " << n << std::endl;
        
      });
    
    this->Add(0, store_protocol_ );

    tx_store_ = new ObjectStore< fetch::chain::Transaction >();
    tx_store_->Load(prefix+"e.db", prefix+"f.db", true);    
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


int main(int argc, char const **argv) 
{
  fetch::logger.DisableLogger();
  commandline::ParamsParser params;
  params.Parse(argc, argv);
  int lane_count =  params.GetParam<int>("lane-count", 1);  
     
  std::string dummy;
  fetch::commandline::DisplayCLIHeader("Multi-lane server");
  
  std::cout << "Starting " << lane_count << " lanes." << std::endl << std::endl;
  

  fetch::network::ThreadManager tm(8);
  std::vector< std::shared_ptr< LaneService > > lanes;
  for(int i = 0 ; i < lane_count ; ++i ) {
    lanes.push_back(std::make_shared< LaneService > (uint32_t(i), uint16_t(8080 + i), tm) );
  }
  
  tm.Start();


  
  std::cout << "Press ENTER to quit" << std::endl;                                       
  std::cin >> dummy;
  
  tm.Stop();


  return 0;
}
