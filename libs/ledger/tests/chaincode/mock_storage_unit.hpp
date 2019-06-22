#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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

    ON_CALL(*this, CurrentHash()).WillByDefault(Invoke(&fake_, &FakeStorageUnit::CurrentHash));
    ON_CALL(*this, LastCommitHash())
        .WillByDefault(Invoke(&fake_, &FakeStorageUnit::LastCommitHash));
    ON_CALL(*this, RevertToHash(_, _))
        .WillByDefault(Invoke(&fake_, &FakeStorageUnit::RevertToHash));
    ON_CALL(*this, Commit(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::Commit));
    ON_CALL(*this, HashExists(_, _)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::HashExists));

    ON_CALL(*this, AddTransaction(_))
        .WillByDefault(Invoke(&fake_, &FakeStorageUnit::AddTransaction));
    ON_CALL(*this, GetTransaction(_, _))
        .WillByDefault(Invoke(&fake_, &FakeStorageUnit::GetTransaction));
    ON_CALL(*this, PollRecentTx(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::PollRecentTx));
  }

  MOCK_METHOD1(Get, Document(ResourceAddress const &));
  MOCK_METHOD1(GetOrCreate, Document(ResourceAddress const &));
  MOCK_METHOD2(Set, void(ResourceAddress const &, StateValue const &));
  MOCK_METHOD1(Lock, bool(ShardIndex));
  MOCK_METHOD1(Unlock, bool(ShardIndex));

  MOCK_METHOD0(CurrentHash, Hash());
  MOCK_METHOD0(LastCommitHash, Hash());
  MOCK_METHOD2(RevertToHash, bool(Hash const &, uint64_t));
  MOCK_METHOD1(Commit, Hash(uint64_t));
  MOCK_METHOD2(HashExists, bool(Hash const &, uint64_t));

  MOCK_METHOD1(AddTransaction, void(fetch::ledger::Transaction const &));
  MOCK_METHOD2(GetTransaction, bool(fetch::ledger::Digest const &, fetch::ledger::Transaction &));
  MOCK_METHOD1(HasTransaction, bool(fetch::byte_array::ConstByteArray const &));
  MOCK_METHOD1(IssueCallForMissingTxs, void(fetch::ledger::DigestSet const &));

  MOCK_METHOD1(PollRecentTx, TxLayouts(uint32_t));

  MOCK_CONST_METHOD0(KeyDump, Keys());
  MOCK_METHOD0(Reset, void());

  FakeStorageUnit &GetFake()
  {
    return fake_;
  }

private:
  FakeStorageUnit fake_;
};
