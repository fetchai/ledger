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
#include "core/random/lfg.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "storage/new_revertible_document_store.hpp"
#include "testing/common_testing_functionality.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <stack>
#include <string>
#include <vector>

using namespace fetch;
using namespace fetch::storage;
using namespace fetch::crypto;
using namespace fetch::testing;

using ByteArray      = fetch::byte_array::ByteArray;
using ConstByteArray = fetch::byte_array::ConstByteArray;

TEST(new_revertible_store_test, basic_example_of_commit_revert1)
{
  NewRevertibleDocumentStore store;
  store.New("a_11.db", "b_11.db", "c_11.db", "d_11.db", true);

  // Keep track of the hashes we get from committing
  std::vector<ByteArray> hashes;

  // Make some changes to the store
  for (std::size_t i = 0; i < 17; ++i)
  {
    std::string set_me{std::to_string(i)};
    store.Set(storage::ResourceAddress(set_me), set_me);
    ASSERT_EQ(store.size(), i + 1);  // this fails for tests using correlated strings
  }

  // Verify state is correct with no changes
  for (std::size_t i = 0; i < 17; ++i)
  {
    // Test for success
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

    // Test for fail
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(10000 + i)));
      EXPECT_EQ(document.failed, true);
      EXPECT_EQ(document.was_created, false);
      EXPECT_NE(ConstByteArray(document), ByteArray(std::to_string(10000 + i)));
    }
  }

  // *** Commit this ***
  hashes.push_back(store.Commit());

  // Verify state is the same
  for (std::size_t i = 0; i < 17; ++i)
  {
    auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
    EXPECT_EQ(document.failed, false);
    EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
  }

  // mash the state
  for (std::size_t i = 0; i < 17; ++i)
  {
    std::string set_me{std::to_string(i)};
    store.Set(storage::ResourceAddress(set_me), std::to_string(i + 5));
  }

  // Verify the change
  for (std::size_t i = 0; i < 17; ++i)
  {
    auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
    EXPECT_EQ(document.failed, false);
    EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i + 5)));
  }

  // EXPECT_EQ(store.HashExists(hashes[0]), true);

  // Revert!
  store.RevertToHash(hashes[0]);

  // Verify old state is as it was
  for (std::size_t i = 0; i < 17; ++i)
  {
    auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
    EXPECT_EQ(document.failed, false);
    EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
  }
}

TEST(DISABLED_new_revertible_store_test, more_involved_commit_revert)
{
  NewRevertibleDocumentStore store;
  store.New("a_12.db", "b_12.db", "c_12.db", "d_12.db", true);

  // Keep track of the hashes we get from committing
  std::vector<ByteArray> hashes;

  // Our keys will be selected to be very close to each other
  auto unique_hashes = GenerateUniqueHashes(1000);

  // Make some changes to the store
  for (std::size_t i = 0; i < unique_hashes.size(); ++i)
  {
    std::string set_me{std::to_string(i)};
    store.Set(storage::ResourceID(unique_hashes[i]), set_me);
    ASSERT_EQ(store.size(), i + 1); /* This fails */
  }

  // Verify state is correct with no changes
  for (std::size_t i = 0; i < unique_hashes.size(); ++i)
  {
    // Test for success
    {
      auto document = store.Get(storage::ResourceID(unique_hashes[i]));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

    // Test for fail
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(10000 + i)));
      EXPECT_EQ(document.failed, true);
      EXPECT_EQ(document.was_created, false);
      EXPECT_NE(ConstByteArray(document), ByteArray(std::to_string(10000 + i)));
    }
  }
}

TEST(new_revertible_store_test, basic_example_of_commit_revert_with_load)
{
  // Keep track of the hashes we get from committing
  std::vector<ByteArray> hashes;

  {
    NewRevertibleDocumentStore store;
    store.New("a_13.db", "b_13.db", "c_13.db", "d_13.db", true);

    // Make some changes to the store
    for (std::size_t i = 0; i < 17; ++i)
    {
      std::string set_me{std::to_string(i)};
      store.Set(storage::ResourceAddress(set_me), set_me);
      ASSERT_EQ(store.size(), i + 1);  // this fails for tests using correlated strings
    }

    // Verify state is correct with no changes
    for (std::size_t i = 0; i < 17; ++i)
    {
      // Test for success
      {
        auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
        EXPECT_EQ(document.failed, false);
        EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
      }

      // Test for fail
      {
        auto document = store.Get(storage::ResourceAddress(std::to_string(10000 + i)));
        EXPECT_EQ(document.failed, true);
        EXPECT_EQ(document.was_created, false);
        EXPECT_NE(ConstByteArray(document), ByteArray(std::to_string(10000 + i)));
      }
    }

    // *** Commit this ***
    hashes.push_back(store.Commit());

    // Verify state is the same
    for (std::size_t i = 0; i < 17; ++i)
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

    // mash the state
    for (std::size_t i = 0; i < 17; ++i)
    {
      std::string set_me{std::to_string(i)};
      store.Set(storage::ResourceAddress(set_me), std::to_string(i + 5));
    }

    // Verify the change
    for (std::size_t i = 0; i < 17; ++i)
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i + 5)));
    }

    // EXPECT_EQ(store.HashExists(hashes[0]), true);

    // Revert!
    store.RevertToHash(hashes[0]);

    // Verify old state is as it was
    for (std::size_t i = 0; i < 17; ++i)
    {
      auto document = store.Get(storage::ResourceAddress(std::to_string(i)));
      EXPECT_EQ(document.failed, false);
      EXPECT_EQ(ConstByteArray(document), ByteArray(std::to_string(i)));
    }

  }
}
