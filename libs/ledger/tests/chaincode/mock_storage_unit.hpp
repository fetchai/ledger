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

#include <gmock/gmock.h>

#include "fake_storage_unit.hpp"

class MockStorageUnit final : public fetch::ledger::StorageUnitInterface
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
    ON_CALL(*this, PollRecentTx(_)).WillByDefault(Invoke(&fake_, &FakeStorageUnit::PollRecentTx));
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

  MOCK_METHOD1(PollRecentTx, std::vector<fetch::chain::TransactionSummary>(uint32_t));

  FakeStorageUnit &GetFake()
  {
    return fake_;
  }

private:
  FakeStorageUnit fake_;
};
