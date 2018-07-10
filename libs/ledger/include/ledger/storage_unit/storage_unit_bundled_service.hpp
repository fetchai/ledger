#ifndef LEDGER_STORAGE_UNIT_BUNDLED_SERVICE_HPP
#define LEDGER_STORAGE_UNIT_BUNDLED_SERVICE_HPP
#include"storage/revertible_document_store.hpp"
#include"storage/document_store_protocol.hpp"
#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"storage/object_store_syncronisation_protocol.hpp"

#include"ledger/storage_unit/storage_unit_interface.hpp"
#include"ledger/storage_unit/lane_service.hpp"

namespace fetch
{
namespace ledger
{

class StorageUnitBundledService 
{
public:
  StorageUnitBundledService(std::string const &dbdir, uint32_t const& lanes, uint16_t const& port, fetch::network::ThreadManager const &tm) {
    for(uint32_t i = 0 ; i < lanes ; ++i ) {
      lanes_.push_back(std::make_shared< LaneService > (dbdir, uint32_t(i), lanes, uint16_t(port + i), tm ) );
    }

  }
  
  ~StorageUnitBundledService() = default;

 
private:
  std::vector< std::shared_ptr< LaneService > > lanes_;
};

}
}

#endif
