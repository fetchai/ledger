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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "storage/key.hpp"
#include "storage/key_value_index.hpp"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <random>
#include <unordered_map>
#include <vector>

namespace {

using namespace fetch;
using namespace fetch::storage;
using CachedKVIndex = KeyValueIndex<KeyValuePair<>, CachedRandomAccessStack<KeyValuePair<>>>;
using KVIndex       = KeyValueIndex<KeyValuePair<>, RandomAccessStack<KeyValuePair<>>>;

struct TestData
{
  byte_array::ByteArray key;
  uint64_t              value;
};

class KeyValueIndexTests : public ::testing::Test
{
public:
  CachedKVIndex cached_kv_index;
  KVIndex       kv_index;

  std::map<byte_array::ConstByteArray, uint64_t> reference;
  fetch::random::LaggedFibonacciGenerator<>      rng;
};

bool ValueConsistency(KeyValueIndexTests &fixture)
{
  std::vector<TestData> values;
  for (std::size_t i = 0; i < 10000; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(fixture.rng() >> 9u);
    }

    if (fixture.reference.find(key) != fixture.reference.end())
    {
      continue;
    }

    fixture.reference[key] = fixture.rng();
    values.push_back({key, fixture.reference[key]});
  }

  fixture.cached_kv_index.New("test1.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    fixture.cached_kv_index.Set(val.key, val.value, val.key);
  }

  bool ok = true;
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val  = values[i];
    uint64_t    data = fixture.cached_kv_index.Get(val.key);
    if (data != val.value)
    {
      std::cout << "Error for " << i << std::endl;
      std::cout << "Expected: " << val.value << std::endl;
      std::cout << "Got: " << data << std::endl;
      return false;
    }
    ok &= (data == val.value);
  }
  return ok;
}

template <typename T1, typename T2>
bool LoadSaveValueConsistency(KeyValueIndexTests &fixture)
{
  std::vector<TestData> values;
  for (std::size_t i = 0; i < 10000; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(fixture.rng() >> 9u);
    }

    if (fixture.reference.find(key) != fixture.reference.end())
    {
      continue;
    }

    fixture.reference[key] = fixture.rng();
    values.push_back({key, fixture.reference[key]});
  }

  T1 index1;
  index1.New("test1.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    index1.Set(val.key, val.value, val.key);
  }

  index1.Close();

  {
    T2 test;
    test.Load("test1.db");
    if (index1.size() != test.size())
    {
      std::cout << "Size mismatch: " << test.size() << " " << index1.size() << std::endl;
      return false;
    }

    if (index1.root_element() != test.root_element())
    {
      std::cout << "Root mismatch: " << test.root_element() << " " << index1.root_element()
                << std::endl;
      return false;
    }

    bool ok = true;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
      auto const &val  = values[i];
      uint64_t    data = test.Get(val.key);

      if (data != val.value)
      {
        std::cout << "Error for " << i << std::endl;
        std::cout << "Expected: " << val.value << std::endl;
        std::cout << "Got: " << data << std::endl;
        return false;
      }
      ok &= (data == val.value);
    }
  }
  return true;
}

TEST_F(KeyValueIndexTests, value_consistency)
{
  EXPECT_TRUE(ValueConsistency(*this));
  EXPECT_TRUE((LoadSaveValueConsistency<KVIndex, KVIndex>(*this)));
  EXPECT_TRUE((LoadSaveValueConsistency<KVIndex, CachedKVIndex>(*this)));
  EXPECT_TRUE((LoadSaveValueConsistency<CachedKVIndex, KVIndex>(*this)));
  EXPECT_TRUE((LoadSaveValueConsistency<CachedKVIndex, CachedKVIndex>(*this)));
}

TEST_F(KeyValueIndexTests, random_insert_hash_consistency)
{
  std::vector<TestData> values;
  for (std::size_t i = 0; i < 10000; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(rng() >> 9u);
    }

    if (reference.find(key) != reference.end())
    {
      continue;
    }

    reference[key] = rng();
    values.push_back({key, reference[key]});
  }

  cached_kv_index.New("test1.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    cached_kv_index.Set(val.key, val.value, val.key);
  }

  auto hash1 = cached_kv_index.Hash();

  // REDO
  cached_kv_index.New("test1.db");

  std::random_device rd;
  std::mt19937       g(rd());
  std::shuffle(values.begin(), values.end(), g);
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    cached_kv_index.Set(val.key, val.value, val.key);
  }
  auto hash2 = cached_kv_index.Hash();

  // REF
  kv_index.New("test1.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    kv_index.Set(val.key, val.value, val.key);
  }

  auto hash3 = cached_kv_index.Hash();

  ASSERT_TRUE(hash1 == hash2);
  ASSERT_TRUE(hash1 == hash3);
}

TEST_F(KeyValueIndexTests, intermediate_flush_hash_consistency)
{
  std::vector<TestData> values;
  for (std::size_t i = 0; i < 1000; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(rng() >> 9u);
    }

    if (reference.find(key) != reference.end())
    {
      continue;
    }

    reference[key] = rng();
    values.push_back({key, reference[key]});
  }

  cached_kv_index.New("test1.db");

  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    cached_kv_index.Set(val.key, val.value, val.key);
  }

  auto hash1 = cached_kv_index.Hash();

  // REDO

  cached_kv_index.New("test1.db");

  std::random_device rd;
  std::mt19937       g(rd());
  std::shuffle(values.begin(), values.end(), g);
  for (std::size_t i = 0; i < values.size(); ++i)
  {

    if ((i % 3) == 0)
    {
      cached_kv_index.Flush();
    }

    auto const &val = values[i];
    cached_kv_index.Set(val.key, val.value, val.key);
  }
  auto hash2 = cached_kv_index.Hash();

  // REF
  kv_index.New("test1.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    kv_index.Set(val.key, val.value, val.key);
  }

  auto hash3 = cached_kv_index.Hash();

  ASSERT_TRUE(hash1 == hash2);
  ASSERT_TRUE(hash1 == hash3);
}

TEST_F(KeyValueIndexTests, double_insertion_hash_consistency)
{
  std::vector<TestData> values;
  for (std::size_t i = 0; i < 10000; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(rng() >> 9u);
    }

    if (reference.find(key) != reference.end())
    {
      continue;
    }

    reference[key] = rng();
    values.push_back({key, reference[key]});
  }

  cached_kv_index.New("test1.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    cached_kv_index.Set(val.key, val.value, val.key);
  }

  std::size_t size1 = cached_kv_index.size();
  auto        hash1 = cached_kv_index.Hash();

  std::random_device rd;
  std::mt19937       g(rd());
  std::shuffle(values.begin(), values.end(), g);
  for (std::size_t i = 0; i < values.size() / 10; ++i)
  {
    auto const &val = values[i];
    cached_kv_index.Set(val.key, val.value, val.key);
  }
  std::size_t size2 = cached_kv_index.size();
  auto        hash2 = cached_kv_index.Hash();

  ASSERT_TRUE(hash1 == hash2);
  ASSERT_TRUE(size1 == size2);
}

TEST_F(KeyValueIndexTests, batched_vs_bulk_load_save_consistency)
{
  std::vector<TestData> values;
  std::size_t           batches    = 10;
  std::size_t           batch_size = 100;
  std::size_t           total      = batches * batch_size;
  std::size_t           i = 0, k = 0;
  while (i < total)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(rng() >> 9u);
    }

    if (reference.find(key) != reference.end())
    {
      continue;
    }

    reference[key] = rng();
    values.push_back({key, reference[key]});
    ++i;
  }

  cached_kv_index.New("test1.db");
  for (std::size_t z = 0; z < values.size(); ++z)
  {
    auto const &val = values[z];
    cached_kv_index.Set(val.key, val.value, val.key);
  }
  std::size_t bulk_size = cached_kv_index.size();
  auto        bulk_hash = cached_kv_index.Hash();

  {
    KVIndex test;
    test.New("test1.db");
  }

  for (std::size_t z = 0; z < batches; ++z)
  {
    KVIndex test;
    test.Load("test1.db");
    for (std::size_t j = 0; j < batch_size; ++j)
    {
      auto const &val = values[k];
      test.Set(val.key, val.value, val.key);
      ++k;
    }
  }

  std::size_t           batched_size;
  byte_array::ByteArray batched_hash;
  {
    KVIndex test;
    test.Load("test1.db");
    batched_hash = test.Hash();
    batched_size = test.size();

    for (std::size_t z = 0; z < test.size(); ++z)
    {
      uint64_t a, b;
      test.GetElement(z, a);
      cached_kv_index.GetElement(z, b);
      ASSERT_FALSE(a != b);
    }
  }

  std::random_device rd;
  std::mt19937       g(rd());
  std::shuffle(values.begin(), values.end(), g);
  {
    KVIndex test;
    test.New("test1.db");
  }
  k = 0;
  for (std::size_t z = 0; z < batches; ++z)
  {
    KVIndex test;
    test.Load("test1.db");
    for (std::size_t j = 0; j < batch_size; ++j)
    {
      auto const &val = values[k];
      test.Set(val.key, val.value, val.key);
      ++k;
    }
  }
  std::size_t           random_batched_size;
  byte_array::ByteArray random_batched_hash;
  {
    KVIndex test;
    test.Load("test1.db");
    random_batched_hash = test.Hash();
    random_batched_size = test.size();
  }

  ASSERT_TRUE(batched_hash == bulk_hash);
  ASSERT_TRUE(batched_hash == random_batched_hash);
  ASSERT_TRUE(bulk_size == batched_size);
  ASSERT_TRUE(bulk_size == random_batched_size);
}

}  // namespace
