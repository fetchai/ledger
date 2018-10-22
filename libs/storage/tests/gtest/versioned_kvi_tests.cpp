//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "storage/key.hpp"
#include "storage/key_value_index.hpp"
#include <gtest/gtest.h>

#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace fetch;
using namespace fetch::storage;

using kvi_type = KeyValueIndex<>;

kvi_type key_Index;

struct TestData
{
  byte_array::ByteArray key;
  uint64_t              value;
};

std::map<byte_array::ConstByteArray, uint64_t> reference1;
fetch::random::LaggedFibonacciGenerator<>      lfgen;

TEST(versioned_kvi_gtest, basic_test)
{
  std::vector<TestData> bookmarks;

  std::vector<TestData> values;
  for (std::size_t i = 0; i < 5; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(lfgen() >> 9);
    }

    if (reference1.find(key) != reference1.end())
    {
      continue;
    }

    reference1[key] = lfgen();
    values.push_back({key, reference1[key]});
  }

  // Insterting data
  key_Index.New("test1.db", "diff.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    key_Index.Set(val.key, val.value, val.key);
    if ((i % 2) == 0)
    {
      byte_array::ByteArray h1 = key_Index.Hash();
      TestData              book;
      book.key   = h1;
      book.value = key_Index.Commit();
      bookmarks.push_back(book);
      if (h1 != key_Index.Hash())
      {
        std::cerr << "Expected hash to be the same before and after commit" << std::endl;
        exit(-1);
      }
    }
  }

  // Checking values
  bool ok = true;
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val  = values[i];
    uint64_t    data = key_Index.Get(val.key);
    ASSERT_EQ(data, val.value);
    ok &= (data == val.value);
  }

  // Reverting
  std::reverse(bookmarks.begin(), bookmarks.end());
  for (auto &b : bookmarks)
  {
    key_Index.Revert(b.value);
    std::cout << "Reverting: " << b.value << " " << byte_array::ToBase64(b.key) << " "
              << byte_array::ToBase64(key_Index.Hash()) << std::endl;
    ASSERT_EQ(b.key, key_Index.Hash());
  }
}
