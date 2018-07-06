#ifndef LEDGER_STORAGE_UNIT_CLIENT_HPP
#define LEDGER_STORAGE_UNIT_CLIENT_HPP
#include"storage/revertible_document_store.hpp"
#include"storage/document_store_protocol.hpp"
#include"storage/object_store.hpp"
#include"storage/object_store_protocol.hpp"
#include"storage/object_store_syncronisation_protocol.hpp"

#include"ledger/storage_unit/storage_unit_interface.hpp"

namespace fetch
{
namespace ledger
{

class StorageUnitBundledService 
{
public:
  StorageUnitBundledService() = default;
  ~StorageUnitBundledService() = default;


};

}
}

