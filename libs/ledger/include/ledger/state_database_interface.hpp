#ifndef FETCH_STATE_DATABASE_INTERFACE_HPP
#define FETCH_STATE_DATABASE_INTERFACE_HPP

#include <storage/document.hpp>
#include "storage/resource_mapper.hpp"

namespace fetch {
namespace ledger {

class StateDatabaseInterface {
public:
  using resource_id_type = storage::ResourceID;
  using document_type = storage::Document;
  using bookmark_type = uint64_t; // TODO: (EJF) This needs to come from the storage implementation

  // Construction / Destruction
  StateDatabaseInterface() = default;
  virtual ~StateDatabaseInterface() = default;

  /// @name State Database Interface
  /// @{
  virtual document_type GetOrCreate(resource_id_type const &rid) = 0;
  virtual document_type Get(resource_id_type const &rid) = 0;
  virtual void Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) = 0;
  virtual bookmark_type Commit(bookmark_type const& b) = 0;
  virtual void Revert(bookmark_type const &b) = 0;
  /// @}
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_INTERFACE_HPP
