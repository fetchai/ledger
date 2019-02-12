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

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "storage/key.hpp"
#include "testing/common_testing_functionality.hpp"

#include <gtest/gtest.h>

using namespace fetch;
using namespace fetch::storage;
using namespace fetch::crypto;
using namespace fetch::testing;

using ByteArray      = fetch::byte_array::ByteArray;
using ConstByteArray = fetch::byte_array::ConstByteArray;
using DefaultKey     = fetch::storage::Key<256>;

// Test that closely correlated keys are found to be unique
TEST(new_key_test, correlated_keys_are_unique)
{
  auto unique_hashes = GenerateUniqueHashes(1000);

  std::vector<DefaultKey> seen_keys;

  for (auto const &hash : unique_hashes)
  {
    for (auto const &key : seen_keys)
    {
      // Check equality operator
      ASSERT_EQ(DefaultKey{hash} == key, false);
    }

    seen_keys.push_back(hash);
  }
}
