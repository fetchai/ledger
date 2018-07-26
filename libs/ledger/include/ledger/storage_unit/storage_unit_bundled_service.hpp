#pragma once
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
  StorageUnitBundledService() = default;  
  ~StorageUnitBundledService() = default;

  void Setup(std::string const &dbdir, std::size_t const& lanes, uint16_t const& port, fetch::network::NetworkManager const &tm, bool start_sync = true) {
    for(std::size_t i = 0 ; i < lanes ; ++i ) {
      lanes_.push_back(
        std::make_shared< LaneService > (dbdir, uint32_t(i), lanes, uint16_t(port + i), tm, start_sync)
      );
    }
  }
 
private:
  std::vector< std::shared_ptr< LaneService > > lanes_;
};

}
}

