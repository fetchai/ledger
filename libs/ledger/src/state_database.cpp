#include "ledger/state_database.hpp"
#include "storage/document_store.hpp"

namespace fetch {
namespace ledger {

bool StateDatabase::Get(resource_id_type const &rid, byte_array::ByteArray &data) {
  return database_.Get(rid, data);
}

bool StateDatabase::Set(resource_id_type const &rid, byte_array::ConstByteArray const &value) {
  return database_.Set(rid, value);
}

StateDatabase::bookmark_type StateDatabase::Commit(bookmark_type const &b) {
  return database_.Commit(b);
}

void StateDatabase::Revert(bookmark_type const &b) {
  database_.Revert(b);
}

} // namespace ledger
} // namespace fetch
