#ifndef FETCH_EXAMPLE_STORAGE_HPP
#define FETCH_EXAMPLE_STORAGE_HPP

#include "core/assert.hpp"
#include "ledger/state_database_interface.hpp"
#include "crypto/fnv.hpp"

#include <unordered_map>

class FakeStateDatabase : public fetch::ledger::StateDatabaseInterface {
public:
  using key_type = fetch::byte_array::ConstByteArray;
  using value_type = fetch::byte_array::ConstByteArray;
  using underlying_storage_type = std::unordered_map<key_type, value_type, fetch::crypto::CallableFNV>;

  document_type GetOrCreate(resource_id_type const &rid) override {
    document_type doc;

    auto it = storage_.find(rid.id());
    if (it != storage_.end()) {
      doc.document = it->second;
    } else {
      doc.was_created = true;
    }

    return doc;
  }

  document_type Get(resource_id_type const &rid) override {
    document_type doc;

    auto it = storage_.find(rid.id());
    if (it != storage_.end()) {
      doc.document = it->second;
    } else {
      doc.failed = true;
    }

    return doc;
  }

  void Set(resource_id_type const &rid, fetch::byte_array::ConstByteArray const& value) override {
    storage_[rid.id()] = value;
  }

  bookmark_type Commit(bookmark_type const& b) override {
    TODO_FAIL("Not implemented");
    return 0;
  }

  void Revert(bookmark_type const &b) override {
    TODO_FAIL("Not implemented");
  }

private:

  underlying_storage_type storage_{};
};

#endif //FETCH_EXAMPLE_STORAGE_HPP
