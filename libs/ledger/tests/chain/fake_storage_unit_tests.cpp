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

#include "core/byte_array/encoders.hpp"

#include "fake_storage_unit.hpp"

#include <gtest/gtest.h>

#include <memory>

using fetch::storage::ResourceAddress;

using FakeStorageUnitPtr = std::unique_ptr<FakeStorageUnit>;

class FakeStorageUnitTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    storage_ = std::make_unique<FakeStorageUnit>();
  }

  void TearDown() override
  {
    storage_.reset();
  }

  void CheckValueIsPresent(char const *key, char const *value)
  {
    auto const result = storage_->Get(ResourceAddress{key});

    ASSERT_FALSE(result.failed);
    ASSERT_FALSE(result.was_created);
    ASSERT_EQ(result.document, value);
  }

  void CheckKeyIsNotPresent(char const *key)
  {
    auto const result = storage_->Get(ResourceAddress{key});

    ASSERT_TRUE(result.failed);
    ASSERT_FALSE(result.was_created);
  }

  FakeStorageUnitPtr storage_;
};

TEST_F(FakeStorageUnitTests, BasicCheck)
{
  storage_->Set(ResourceAddress{"key 1"}, "value 1");
  CheckValueIsPresent("key 1", "value 1");

  // create the commit
  auto const state1 = storage_->Commit();
  ASSERT_EQ(storage_->LastCommitHash(), state1);

  storage_->Set(ResourceAddress{"key 2"}, "value 2");
  CheckValueIsPresent("key 1", "value 1");
  CheckValueIsPresent("key 2", "value 2");

  auto const state2 = storage_->Commit();
  ASSERT_EQ(storage_->LastCommitHash(), state2);

  storage_->Set(ResourceAddress{"key 3"}, "value 3");
  CheckValueIsPresent("key 1", "value 1");
  CheckValueIsPresent("key 2", "value 2");
  CheckValueIsPresent("key 3", "value 3");

  auto const state3 = storage_->Commit();
  ASSERT_EQ(storage_->LastCommitHash(), state3);

  // revert back to state 1
  ASSERT_TRUE(storage_->RevertToHash(state1));
  ASSERT_EQ(storage_->LastCommitHash(), state1);
  ASSERT_TRUE(storage_->HashExists(state1));
  ASSERT_FALSE(storage_->HashExists(state2));
  ASSERT_FALSE(storage_->HashExists(state3));

  CheckValueIsPresent("key 1", "value 1");
  CheckKeyIsNotPresent("key 2");
  CheckKeyIsNotPresent("key 3");

  storage_->Set(ResourceAddress{"key 4"}, "value 4");
  CheckValueIsPresent("key 1", "value 1");
  CheckKeyIsNotPresent("key 2");
  CheckKeyIsNotPresent("key 3");
  CheckValueIsPresent("key 4", "value 4");

  auto const state4 = storage_->Commit();
  ASSERT_EQ(storage_->LastCommitHash(), state4);

  //  auto const commit =
  //
  //  storage_->Set(ResourceAddress{"key 2"}, "value 2");
}