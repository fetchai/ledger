#pragma once

#include "fake_storage_unit.hpp"

#include <gmock/gmock.h>

class MockStorageUnit : public fetch::ledger::StorageUnitInterface
{
public:
  MockStorageUnit()
  {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, Get(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Get));
    ON_CALL(*this, GetOrCreate(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::GetOrCreate));
    ON_CALL(*this, Set(_, _)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Set));
    ON_CALL(*this, Lock(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Lock));
    ON_CALL(*this, Unlock(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Unlock));
    ON_CALL(*this, Hash()).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Hash));
    ON_CALL(*this, Commit(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Commit));
    ON_CALL(*this, Revert(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Revert));
    ON_CALL(*this, AddTransaction(_))
        .WillByDefault(Invoke(&fake_, &FakeStorageUnit::AddTransaction));
    ON_CALL(*this, GetTransaction(_, _))
        .WillByDefault(Invoke(&fake_, &FakeStorageUnit::GetTransaction));
  }

  MOCK_METHOD1(Get, Document(ResourceAddress const &));
  MOCK_METHOD1(GetOrCreate, Document(ResourceAddress const &));
  MOCK_METHOD2(Set, void(ResourceAddress const &, StateValue const &));
  MOCK_METHOD1(Lock, bool(ResourceAddress const &));
  MOCK_METHOD1(Unlock, bool(ResourceAddress const &));
  MOCK_METHOD0(Hash, hash_type());
  MOCK_METHOD1(Commit, void(bookmark_type const &));
  MOCK_METHOD1(Revert, void(bookmark_type const &));

  MOCK_METHOD1(AddTransaction, void(fetch::chain::Transaction const &));
  MOCK_METHOD2(GetTransaction,
               bool(fetch::byte_array::ConstByteArray const &, fetch::chain::Transaction &));

  FakeStorageUnit &GetFake() { return fake_; }

private:
  FakeStorageUnit fake_;
};
