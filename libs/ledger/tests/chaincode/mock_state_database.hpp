#ifndef FETCH_MOCK_STATE_DATABASE_HPP
#define FETCH_MOCK_STATE_DATABASE_HPP

#include "fake_state_database.hpp"

#include <gmock/gmock.h>

class MockStateDatabase : public fetch::ledger::StateDatabaseInterface {
public:

  MockStateDatabase() {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, GetOrCreate(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::GetOrCreate));
    ON_CALL(*this, Get(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Get));
    ON_CALL(*this, Set(_, _))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Set));
    ON_CALL(*this, Lock(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Lock));
    ON_CALL(*this, Unlock(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Unlock));
    ON_CALL(*this, HasLock(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::HasLock));
    ON_CALL(*this, Hash())
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Hash));
    ON_CALL(*this, Commit(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Commit));
    ON_CALL(*this, Revert(_))
      .WillByDefault(Invoke(&fake_, &FakeStateDatabase::Revert));
  }

  MOCK_METHOD1(GetOrCreate, document_type(resource_id_type const &));
  MOCK_METHOD1(Get, document_type(resource_id_type const &));
  MOCK_METHOD2(Set, void(resource_id_type const &, fetch::byte_array::ConstByteArray const &));
  MOCK_METHOD1(Lock, void(resource_id_type const &));
  MOCK_METHOD1(Unlock, void(resource_id_type const &));
  MOCK_METHOD1(HasLock, void(resource_id_type const &));
  MOCK_METHOD0(Hash, hash_type());
  MOCK_METHOD1(Commit, bookmark_type(bookmark_type const &));
  MOCK_METHOD1(Revert, void(bookmark_type const &));

private:

  FakeStateDatabase fake_;
};

#endif //FETCH_MOCK_STATE_DATABASE_HPP
