#ifndef FETCH_FAKE_STORAGE_UNIT_HPP
#define FETCH_FAKE_STORAGE_UNIT_HPP

#include "crypto/fnv.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <unordered_set>
#include <unordered_map>

class FakeStorageUnit : public fetch::ledger::StorageUnitInterface {
public:
  using transaction_store_type = std::unordered_map<fetch::byte_array::ConstByteArray,fetch::chain::Transaction, fetch::crypto::CallableFNV>;
  using state_store_type = std::unordered_map<fetch::byte_array::ConstByteArray, fetch::byte_array::ConstByteArray, fetch::crypto::CallableFNV>;
  using lock_store_type = std::unordered_set<fetch::byte_array::ConstByteArray, fetch::crypto::CallableFNV>;

  document_type GetOrCreate(fetch::byte_array::ConstByteArray const &key) override {
    document_type doc;

    auto it = state_.find(key);
    if (it != state_.end()) {
      doc.document = it->second;
    } else {
      doc.was_created = true;
    }

    return doc;
  }


  document_type Get(fetch::byte_array::ConstByteArray const &key) override {
    document_type doc;

    auto it = state_.find(key);
    if (it != state_.end()) {
      doc.document = it->second;
    } else {
      doc.failed = true;
    }

    return doc;
  }

  void Set(fetch::byte_array::ConstByteArray const &key, fetch::byte_array::ConstByteArray const &value) override {
    state_[key] = value;
  }

  bool Lock(fetch::byte_array::ConstByteArray const &key) override {
    bool success = false;

    bool const already_locked = locks_.find(key) != locks_.end();
    if (!already_locked) {
      locks_.insert(key);
      success = true;
    }

    return success;
  }

  bool Unlock(fetch::byte_array::ConstByteArray const &key) override {
    bool success = false;

    bool const already_locked = locks_.find(key) != locks_.end();
    if (already_locked) {
      locks_.erase(key);
      success = true;
    }

    return success;
  }

  void AddTransaction(fetch::chain::Transaction const &tx) override {
    transactions_[tx.digest()] = tx;
  }

  bool GetTransaction(fetch::byte_array::ConstByteArray const &digest, fetch::chain::Transaction &tx) override {
    bool success = false;

    auto it = transactions_.find(digest);
    if (it != transactions_.end()) {
      tx = it->second;
      success = true;
    }

    return success;
  }

  hash_type Hash() override {
    TODO_FAIL("Not implemented");
    return {};
  }

  void Commit(bookmark_type const &bookmark) override {
    TODO_FAIL("Not implemented");
  }

  void Revert(bookmark_type const &bookmark) override {
    TODO_FAIL("Not implemented");
  }

private:

  transaction_store_type transactions_;
  state_store_type state_;
  lock_store_type locks_;
};

#endif //FETCH_FAKE_STORAGE_UNIT_HPP
