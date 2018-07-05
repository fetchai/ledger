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
  using hash_type = int;

  // Construction / Destruction
  StateDatabaseInterface() = default;
  virtual ~StateDatabaseInterface() = default;

  /// @name State Database Interface
  /// @{
  virtual document_type GetOrCreate(resource_id_type const &rid) = 0;
  virtual document_type Get(resource_id_type const &rid) = 0;
  virtual void Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) = 0;
  virtual void Lock(resource_id_type const &rid) = 0;
  virtual void Unlock(resource_id_type const &rid) = 0;
  virtual void HasLock(resource_id_type const &rid) = 0;
  virtual hash_type Hash() = 0;
  virtual bookmark_type Commit(bookmark_type const& b) = 0;
  virtual void Revert(bookmark_type const &b) = 0;
  /// @}
};

template <typename T>
class ObjectStoreInterface {
public:
  using key_type = byte_array::ConstByteArray;
  using object_type = T;

  ObjectStoreInterface() = default;
  virtual ~ObjectStoreInterface() = default;

  /// @name Object Store Interface
  /// @{
  virtual bool GetOrCreate(key_type const &rid, object_type &value) = 0;
  virtual void Get(key_type const &rid, object_type &value) = 0;
  virtual void Set(key_type const &rid, object_type const& value) = 0;
  /// @}
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_INTERFACE_HPP
