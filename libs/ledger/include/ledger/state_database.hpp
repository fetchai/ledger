#ifndef FETCH_STATE_DATABASE_HPP
#define FETCH_STATE_DATABASE_HPP

#include "ledger/state_database_interface.hpp"
#include "storage/document_store.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class StateDatabase : public StateDatabaseInterface {
public:
  using database_type = storage::RevertibleDocumentStore;

  // Construction / Destruction
  StateDatabase() = default;
  ~StateDatabase() override = default;

  /// @name State Database
  /// @{
  document_type GetOrCreate(resource_id_type const &rid) override {
    return database_.GetOrCreate(rid);
  }

  document_type Get(resource_id_type const &rid) override {
    return database_.Get(rid);
  }

  void Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) override {
    database_.Set(rid, value);
  }

  bookmark_type Commit(bookmark_type const& b) override {
    return database_.Commit(b);
  }

  void Revert(bookmark_type const &b) override {
    database_.Revert(b);
  }
  /// @}

private:

  database_type database_{};
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_HPP
