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
#include "storage/new_versioned_random_access_stack.hpp"
#include "testing/common_testing_functionality.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace fetch;
using namespace fetch::storage;
using namespace fetch::crypto;
using namespace fetch::testing;

using ByteArray = fetch::byte_array::ByteArray;

TEST(versioned_random_access_stack_gtest, basic_example_of_commit_revert2)
{
  NewVersionedRandomAccessStack<StringProxy> stack;
  stack.New("b_main.db", "b_history.db");

  // Make some changes to the stack
  for (std::size_t i = 0; i < 17; ++i)
  {
    stack.Push(std::to_string(i));
  }

  // Verify state is correct with no changes
  for (std::size_t i = 0; i < 17; ++i)
  {
    EXPECT_NE(stack.Get(i), std::to_string(i + 11));  // counter check
    EXPECT_EQ(stack.Get(i), std::to_string(i));
  }

  // Create a bunch of hashes we want to bookmark with
  std::vector<ByteArray> hashes;

  for (std::size_t i = 0; i < 4; ++i)
  {
    hashes.push_back(Hash<crypto::SHA256>(std::to_string(i)));
  }

  // *** Commit this ***
  stack.Commit(hashes[0]);

  // Verify state is the same
  for (std::size_t i = 0; i < 17; ++i)
  {
    EXPECT_EQ(stack.Get(i), std::to_string(i));
  }

  // mash the state
  for (std::size_t i = 0; i < 17; ++i)
  {
    stack.Set(i, std::to_string(i + 5));
  }

  // Verify the change
  for (std::size_t i = 0; i < 17; ++i)
  {
    EXPECT_EQ(stack.Get(i), std::to_string(i + 5));
  }

  // EXPECT_EQ(stack.HashExists(hashes[0]), true);

  // Revert!
  stack.RevertToHash(hashes[0]);

  // Verify old state is as it was
  for (std::size_t i = 0; i < 17; ++i)
  {
    EXPECT_EQ(stack.Get(i), std::to_string(i));
  }
}

TEST(versioned_random_access_stack_gtest, try_to_revert_to_bad_hash)
{
  NewVersionedRandomAccessStack<StringProxy> stack;
  stack.New("b_main.db", "b_history.db");

  // Create a bunch of hashes we want to bookmark with
  std::vector<ByteArray> hashes;

  for (std::size_t i = 0; i < 4; ++i)
  {
    hashes.push_back(Hash<crypto::SHA256>(std::to_string(i)));
  }

  // Make some changes to the stack
  for (std::size_t i = 0; i < 17; ++i)
  {
    stack.Push(std::to_string(i));
  }

  // Verify state is correct with no changes
  for (std::size_t i = 0; i < 17; ++i)
  {
    StringProxy a;
    a = stack.Get(i);
    StringProxy b;
    b = StringProxy(std::to_string(i));

    bool res = a == b;

    EXPECT_EQ(a, b) << "thing";
    EXPECT_EQ(res, true) << "thing";

    EXPECT_NE(stack.Get(i), std::to_string(i + 11));  // counter check
    EXPECT_EQ(stack.Get(i), std::to_string(i));
  }

  // *** Commit this ***
  stack.Commit(hashes[0]);

  // Revert to bad hash
  ASSERT_THROW(stack.RevertToHash(hashes[1]), StorageException);
}

TEST(versioned_random_access_stack_gtest, loading_file)
{
  // Create a bunch of hashes we want to bookmark with
  std::vector<ByteArray> hashes;

  for (std::size_t i = 0; i < 4; ++i)
  {
    hashes.push_back(Hash<crypto::SHA256>(std::to_string(i)));
  }

  {
    NewVersionedRandomAccessStack<StringProxy> stack;
    stack.New("c_main.db", "c_history.db");

    // Make some changes to the stack
    for (std::size_t i = 0; i < 17; ++i)
    {
      stack.Push(std::to_string(i));
    }

    // Verify state is correct with no changes
    for (std::size_t i = 0; i < 17; ++i)
    {
      EXPECT_NE(stack.Get(i), std::to_string(i + 11));  // counter check
      EXPECT_EQ(stack.Get(i), std::to_string(i));
    }

    // *** Commit this ***
    stack.Commit(hashes[0]);

    // Verify state is the same
    for (std::size_t i = 0; i < 17; ++i)
    {
      EXPECT_EQ(stack.Get(i), std::to_string(i));
    }

    // mash the state
    for (std::size_t i = 0; i < 17; ++i)
    {
      stack.Set(i, std::to_string(i + 5));
    }
  }

  {
    NewVersionedRandomAccessStack<StringProxy> stack;
    stack.Load("c_main.db", "c_history.db");

    // Verify the change is correct after loading the file up
    for (std::size_t i = 0; i < 17; ++i)
    {
      EXPECT_EQ(stack.Get(i), std::to_string(i + 5));
    }

    // Revert!
    stack.RevertToHash(hashes[0]);

    // Verify old state is as it was
    for (std::size_t i = 0; i < 17; ++i)
    {
      EXPECT_EQ(stack.Get(i), std::to_string(i));
    }
  }
}
