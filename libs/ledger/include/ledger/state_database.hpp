#ifndef FETCH_STATE_DATABASE_HPP
#define FETCH_STATE_DATABASE_HPP

#include "ledger/state_database_interface.hpp"
#include "storage/document_store.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class StateDatabase : public StateDatabaseInterface {
public:
  using database_type = storage::DocumentStore<>;

  // Construction / Destruction
  StateDatabase() = default;
  ~StateDatabase() override = default;

  /// @name State Database
  /// @{
  bool Get(resource_id_type const &rid, byte_array::ByteArray &data) override;
  bool Set(resource_id_type const &rid, byte_array::ConstByteArray const &value) override;
  bookmark_type Commit(bookmark_type const &b) override;
  void Revert(bookmark_type const &b) override;
  /// @}

private:

  database_type database_{};
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_STATE_DATABASE_HPP
