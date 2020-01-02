//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "storage/resource_mapper.hpp"

#include "gmock/gmock.h"

namespace {

using fetch::ledger::CachedStorageAdapter;
using fetch::ledger::StorageInterface;
using fetch::storage::ResourceAddress;
using fetch::storage::Document;

using testing::Return;

class MockStorage : public StorageInterface
{
public:
  MOCK_CONST_METHOD1(Get, Document(ResourceAddress const &));
  MOCK_METHOD1(GetOrCreate, Document(ResourceAddress const &));
  MOCK_METHOD2(Set, void(ResourceAddress const &, StateValue const &));
  MOCK_METHOD1(Lock, bool(ShardIndex));
  MOCK_METHOD1(Unlock, bool(ShardIndex));
  MOCK_METHOD0(Reset, void());
};

class CachedStorageAdapterTests : public testing::Test
{
public:
  CachedStorageAdapterTests()
    : cached_storage_adapter{mock_storage}
  {}

  ResourceAddress key{"key"};

  MockStorage          mock_storage{};
  CachedStorageAdapter cached_storage_adapter;
};

TEST_F(CachedStorageAdapterTests, GetOrCreate_caches_result_if_retrieval_from_storage_succeeds)
{
  Document doc;
  doc.failed = false;

  EXPECT_CALL(mock_storage, GetOrCreate(key)).WillOnce(Return(doc));

  cached_storage_adapter.GetOrCreate(key);
  cached_storage_adapter.GetOrCreate(key);
}

TEST_F(CachedStorageAdapterTests, GetOrCreate_does_not_cache_result_if_retrieval_from_storage_fails)
{
  Document doc;
  doc.failed = true;

  EXPECT_CALL(mock_storage, GetOrCreate(key)).Times(2).WillRepeatedly(Return(doc));

  cached_storage_adapter.GetOrCreate(key);
  cached_storage_adapter.GetOrCreate(key);
}

TEST_F(CachedStorageAdapterTests, Get_caches_result_if_retrieval_from_storage_succeeds)
{
  Document doc;
  doc.failed = false;

  EXPECT_CALL(mock_storage, Get(key)).WillOnce(Return(doc));

  cached_storage_adapter.Get(key);
  cached_storage_adapter.Get(key);
}

TEST_F(CachedStorageAdapterTests, Get_does_not_cache_result_if_retrieval_from_storage_fails)
{
  Document doc;
  doc.failed = true;

  EXPECT_CALL(mock_storage, Get(key)).Times(2).WillRepeatedly(Return(doc));

  cached_storage_adapter.Get(key);
  cached_storage_adapter.Get(key);
}

}  // namespace
