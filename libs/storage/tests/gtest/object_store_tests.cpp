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

#include "core/byte_array/decoders.hpp"
#include "core/random/lfg.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/base_types.hpp"
#include "storage/object_store.hpp"
#include "testing/common_testing_functionality.hpp"


#include "gtest/gtest.h"

#include <algorithm>

using namespace fetch::storage;
using namespace fetch::byte_array;
using namespace fetch::testing;

/**
 * Test class used to verify that the object store can ser/deser objects correctly
 */
struct TestSerDeser
{
  int         first;
  uint64_t    second;
  std::string third;

  /**
   * Comparison operator
   *
   * @param: rhs Other test class
   *
   * @return: less than
   */
  bool operator<(TestSerDeser const &rhs) const
  {
    return third < rhs.third;
  }

  /**
   * Equality operator
   *
   * @param: rhs Other test class
   *
   * @return: is equal to
   */
  bool operator==(TestSerDeser const &rhs) const
  {
    return first == rhs.first && second == rhs.second && third == rhs.third;
  }
};

namespace fetch
{
namespace serializers
{
template< typename D >
struct ArraySerializer< TestSerDeser, D >
{
public: 
  using Type = TestSerDeser;
  using DriverType = D;

  template< typename Constructor >
  static void Serialize(Constructor & array_constructor, Type const & b)
  {
    auto array = array_constructor(3);
    array.Append(b.first);
    array.Append(b.second);
    array.Append(b.third);
  }

  template< typename ArrayDeserializer >
  static void Deserialize(ArrayDeserializer & array, Type & b)
  {
    if(array.size() != 3)
    {
      throw SerializableException(std::string("expected 3 elements."));
    }

    array.GetNextValue(b.first);
    array.GetNextValue(b.second);
    array.GetNextValue(b.third);    
  }  

};
}
}

TEST(storage_object_store_basic_functionality, Setting_and_getting_elements)
{
  for (uint64_t iterations = 3; iterations < 10; ++iterations)
  {
    ObjectStore<uint64_t> testStore;
    testStore.New("testFile.db", "testIndex.db");

    for (uint64_t i = 0; i < iterations; ++i)
    {
      testStore.Set(ResourceAddress(std::to_string(i)), i);

      uint64_t result;

      testStore.Get(ResourceAddress(std::to_string(i)), result);

      EXPECT_EQ(i, result);
    }

    // Do a second run
    for (uint64_t i = 0; i < iterations; ++i)
    {
      uint64_t result;

      testStore.Get(ResourceAddress(std::to_string(i)), result);

      EXPECT_EQ(i, result);
    }

    // Check against false positives
    for (uint64_t i = 1; i < iterations; ++i)
    {
      uint64_t result = 0;

      testStore.Get(ResourceAddress(std::to_string(i + iterations)), result);

      // Suppress most expects to avoid spamming terminal
      if (0 != result || i == 1)
      {
        EXPECT_EQ(0, result);
      }
    }
  }
}

TEST(storage_object_store_basic_functionality, find_over_basic_struct)
{
  std::vector<uint64_t> keyTests{99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};

  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    std::vector<testType>                     objectsCopy;
    fetch::random::LaggedFibonacciGenerator<> lfg;

    // Create vector of random numbers
    for (uint64_t i = 0; i < numberOfKeys; ++i)
    {
      uint64_t random = lfg();

      testType test;
      test.first  = int(-random);
      test.second = random;

      test.third = std::to_string(random);

      testStore.Set(ResourceAddress(test.third), test);
      objects.push_back(test);
    }

    std::sort(objects.begin(), objects.end());

    // Test successfully finding and testing to find elements
    bool successfullyFound = true;
    int  index             = 0;
    for (auto const &i : objects)
    {
      auto it = testStore.Find(ResourceAddress(i.third));
      if (it == testStore.end())
      {
        successfullyFound = false;
        break;
      }

      index++;
    }

    EXPECT_TRUE(successfullyFound);

    successfullyFound = false;

    for (uint64_t i = 0; i < 100; ++i)
    {
      auto it = testStore.Find(ResourceAddress(std::to_string(lfg())));

      if (it != testStore.end())
      {
        successfullyFound = true;
        break;
      }
    }

    EXPECT_FALSE(successfullyFound);
  }
}

TEST(storage_object_store_with_STL_gtest, find_over_basic_struct_expect_failures)
{
  std::vector<uint64_t> keyTests{99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};

  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    // Create vector of random numbers
    for (uint64_t i = 0; i < numberOfKeys; ++i)
    {
      testType test;
      test.first  = int(-i);
      test.second = i;

      test.third = std::to_string(i);

      testStore.Set(ResourceAddress(test.third), test);
    }

    bool successfullyFound = false;

    // Expect in the case of hash collisions, we shouldn't find these
    for (uint64_t i = numberOfKeys + 1; i < numberOfKeys * 2; ++i)
    {
      auto it = testStore.Find(ResourceAddress(std::to_string(i)));
      if (it != testStore.end())
      {
        successfullyFound = true;
        break;
      }
    }

    EXPECT_FALSE(successfullyFound);
  }
}

TEST(storage_object_store_with_STL_gtest, iterator_over_basic_struct)
{
  std::vector<uint64_t> keyTests{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 99, 100, 1010, 9999};
  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    std::vector<testType>                     objectsCopy;
    fetch::random::LaggedFibonacciGenerator<> lfg;

    // Create vector of random numbers
    for (uint64_t i = 0; i < numberOfKeys; ++i)
    {
      uint64_t random = lfg();

      testType test;
      test.first  = int(-random);
      test.second = random;

      test.third = std::to_string(random);

      testStore.Set(ResourceAddress(test.third), test);
      objects.push_back(test);
    }

    std::sort(objects.begin(), objects.end());

    auto it = testStore.begin();
    while (it != testStore.end())
    {
      objectsCopy.push_back(*it);
      ++it;
    }

    std::sort(objectsCopy.begin(), objectsCopy.end());

    EXPECT_EQ(objectsCopy.size(), objects.size());
    bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
    EXPECT_TRUE(allMatch);
  }
}

TEST(storage_object_store_with_STL_gtest,
     subtree_iterator_over_basic_struct_1_to_8_bits_root_sizes_split)
{
  std::vector<uint64_t> keyTests{0,  1,  2,  3,  4,  5,  6,   7,   8,   9,
                                    10, 11, 12, 13, 14, 99, 100, 133, 998, 1001};
  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    fetch::random::LaggedFibonacciGenerator<> lfg;

    ByteArray array;
    array.Resize(256 / 8);

    for (uint64_t i = 0; i < array.size(); ++i)
    {
      array[i] = 0;
    }

    // Create vector of random numbers
    for (uint64_t i = 0; i < numberOfKeys; ++i)
    {
      uint64_t random = lfg();

      testType test;
      test.first  = int(-random);
      test.second = random;

      test.third = std::to_string(random);

      testStore.Set(ResourceAddress(test.third), test);
      objects.push_back(test);
    }

    for (std::uint8_t root_size_in_bits{1}; root_size_in_bits <= 8; ++root_size_in_bits)
    {
      std::vector<testType> objectsCopy;

      // Now, aim to split the store up and copy it across perfectly
      for (uint64_t root = 0, end = (1u << root_size_in_bits); root < end; ++root)
      {
        array[0] = static_cast<uint8_t>(root);

        auto rid = ResourceID(array);

        auto it = testStore.GetSubtree(rid, root_size_in_bits);

        while (it != testStore.end())
        {
          objectsCopy.push_back(*it);
          ++it;
        }
      }

      std::cout << "Test for " << static_cast<uint64_t>(root_size_in_bits)
                << " bits long root, and " << numberOfKeys
                << " number of elements stored in storage." << std::endl;
      // expect iterator test to go well
      EXPECT_EQ(objects.size(), objectsCopy.size());

      std::sort(objects.begin(), objects.end());
      std::sort(objectsCopy.begin(), objectsCopy.end());

      bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
      EXPECT_TRUE(allMatch);
    }
  }
}

TEST(storage_object_store, correlated_strings_work_correctly)
{
  ObjectStore<std::string> testStore;
  testStore.New("testFile_01.db", "testIndex_01.db");

  auto     unique_ids    = GenerateUniqueIDs(256);
  uint64_t expected_size = 0;

  // Set each key to itself as a string
  for (auto const &id : unique_ids)
  {
    testStore.Set(id, id.ToString());
    EXPECT_EQ(testStore.size(), ++expected_size);
  }

  ASSERT_EQ(testStore.size(), unique_ids.size()) << "ERROR: Failed to verify final size!";
}

TEST(storage_object_store_with_STL_gtest, iterator_over_basic_struct_with_key_info)
{
  std::vector<uint64_t> keyTests{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 99, 100, 1010, 9999};
  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::set<ResourceID> all_keys;
    std::set<ResourceID> all_keys_verify;

    fetch::random::LaggedFibonacciGenerator<> lfg;

    // Create vector of random numbers
    for (uint64_t i = 0; i < numberOfKeys; ++i)
    {
      uint64_t random = lfg();

      testType test;
      test.first  = int(-random);
      test.second = random;

      test.third = std::to_string(random);

      all_keys.insert(
          ResourceAddress(std::to_string(i)));  // Set of all the keys in our store is created
      testStore.Set(ResourceAddress(std::to_string(i)), test);
    }

    ASSERT_EQ(all_keys.size(), numberOfKeys);

    auto it = testStore.begin();
    while (it != testStore.end())
    {
      ResourceID key = it.GetKey();

      EXPECT_EQ(all_keys_verify.find(key) == all_keys_verify.end(), true);

      all_keys_verify.insert(key);

      ++it;
    }

    EXPECT_EQ(all_keys_verify.size(), all_keys.size());
  }
}
