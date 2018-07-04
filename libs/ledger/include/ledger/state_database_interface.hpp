#ifndef FETCH_STATE_DATABASE_INTERFACE_HPP
#define FETCH_STATE_DATABASE_INTERFACE_HPP

#include "storage/resource_mapper.hpp"

namespace fetch {
namespace ledger {

class StateDatabaseInterface {
public:
  using resource_id_type = storage::ResourceID;
  using bookmark_type = uint64_t; // TODO: (EJF) This needs to come from the storage implementation

  virtual ~StateDatabaseInterface() = default;

  /// @name State Database Interface
  /// @{
  virtual bool Get(resource_id_type const &rid, byte_array::ByteArray &data) = 0;
  virtual bool Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) = 0;
  virtual bookmark_type Commit(bookmark_type const& b) = 0;
  virtual void Revert(bookmark_type const &b) = 0;
  /// @}
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_INTERFACE_HPP
