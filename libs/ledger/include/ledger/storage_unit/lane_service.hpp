#ifndef LEDGER_STORAGE_UNIT_LANE_SERVICE_HPP
#define LEDGER_STORAGE_UNIT_LANE_SERVICE_HPP
#include"storage/revertible_document_store.hpp"
#include"storage/document_store_protocol.hpp"
#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"storage/object_store_syncronisation_protocol.hpp"
#include"network/service/server.hpp"
#include"ledger/storage_unit/lane_controller.hpp"
#include"ledger/storage_unit/lane_controller_protocol.hpp"
#include"ledger/storage_unit/lane_connectivity_details.hpp"
#include"ledger/chain/transaction.hpp"
#include"network/tcp/connection_register.hpp"

namespace fetch
{
namespace ledger
{

class LaneService : public service::ServiceServer< fetch::network::TCPServer >
{
public:
  using connectivity_details_type = LaneConnectivityDetails;    
  using document_store_type = storage::RevertibleDocumentStore;
  using document_store_protocol_type = storage::RevertibleDocumentStoreProtocol;  
  using transaction_store_type = storage::ObjectStore<fetch::chain::Transaction>;
  using transaction_store_protocol_type = storage::ObjectStoreProtocol<fetch::chain::Transaction>;
  using client_register_type = fetch::network::ConnectionRegister< connectivity_details_type >;  
  using controller_type = LaneController;
  using controller_protocol_type = LaneControllerProtocol; 
  using connection_handle_type = client_register_type::connection_handle_type; 
  using super_type = service::ServiceServer< fetch::network::TCPServer >;
  
  enum {
    STATE = 1,
    TX_STORE,
    TX_STORE_SYNC,
    SLICE_STORE,
    SLICE_STORE_SYNC,
    CONTROLLER
  };
  
  // TODO: Make config JSON
  LaneService(uint32_t const &lane, uint32_t const &max_lanes,
    uint16_t port, fetch::network::ThreadManager tm)
    : super_type(port, tm) {

    std::stringstream s;
    s << "lane" << lane << "_";
    std::string prefix = s.str();    

    // TX Store
    tx_store_ = new transaction_store_type();    
    tx_store_->Load(prefix+"e.db", prefix+"f.db", true);

//    tx_sync_ = new ObjectStoreSyncronisation< fetch::chain::Transaction >( tx_store_ );
//    tx_sync_protocol_ = new ObjectStoreSyncronisationProtocol< fetch::chain::Transaction >(tx_sync_ );
    
    tx_store_protocol_ = new transaction_store_protocol_type(tx_store_);
    this->Add(TX_STORE, tx_store_protocol_ );

       
    // State DB
    state_db_ = new document_store_type();  
    state_db_->Load(prefix+"a.db", prefix+"b.db", prefix+"c.db", prefix+"d.db", true);
    
    state_db_protocol_ = new document_store_protocol_type(state_db_, lane, max_lanes);        
    this->Add(STATE, state_db_protocol_ );

    // Controller
    controller_ = new controller_type(register_, tm);
    controller_protocol_ = new controller_protocol_type(controller_);
    this->Add(CONTROLLER, controller_protocol_ );    
    
  }
  
  ~LaneService() 
  {
    // TODO: Remove protocol
    delete state_db_protocol_;
    delete state_db_;

    // TODO: Remove protocol
    delete tx_store_protocol_;
    delete tx_store_;
//    delete tx_sync_;    
//    delete tx_sync_protocol_;

    // TODO: Remove protocol
    delete controller_protocol_;    
    delete controller_;    
  }
  
private:
  client_register_type register_;
  
  controller_type *controller_;
  controller_protocol_type *controller_protocol_;  
  
  
  document_store_type *state_db_;
  document_store_protocol_type *state_db_protocol_;

  transaction_store_type *tx_store_;
  transaction_store_protocol_type *tx_store_protocol_;

//  ObjectStoreSyncronisation< fetch::chain::Transaction > *tx_sync_;  
//  ObjectStoreSyncronisationProtocol< fetch::chain::Transaction > *tx_sync_protocol_;


//  ObjectStoreP2PController< fetch::chain::Transaction > *p2p_controller_;  
};

}
}

#endif
