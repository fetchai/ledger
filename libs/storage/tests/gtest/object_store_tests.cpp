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

/**
 * Serializer for TestSerDeser
 *
 * @param: serializer The serializer
 * @param: b The class to serialize
 */
template <typename T>
inline void Serialize(T &serializer, TestSerDeser const &b)
{
  serializer << b.first;
  serializer << b.second;
  serializer << b.third;
}

/**
 * Deserializer for TestSerDeser
 *
 * @param: serializer The deserializer
 * @param: b The class to deserialize
 */
template <typename T>
inline void Deserialize(T &serializer, TestSerDeser &b)
{
  serializer >> b.first;
  serializer >> b.second;
  std::string ret;
  serializer >> ret;
  b.third = ret;
}

TEST(storage_object_store_basic_functionality, Setting_and_getting_elements)
{
  for (std::size_t iterations = 3; iterations < 10; ++iterations)
  {
    ObjectStore<std::size_t> testStore;
    testStore.New("testFile.db", "testIndex.db");

    for (std::size_t i = 0; i < iterations; ++i)
    {
      testStore.Set(ResourceAddress(std::to_string(i)), i);

      std::size_t result;

      testStore.Get(ResourceAddress(std::to_string(i)), result);

      EXPECT_EQ(i, result);
    }

    // Do a second run
    for (std::size_t i = 0; i < iterations; ++i)
    {
      std::size_t result;

      testStore.Get(ResourceAddress(std::to_string(i)), result);

      EXPECT_EQ(i, result);
    }

    // Check against false positives
    for (std::size_t i = 1; i < iterations; ++i)
    {
      std::size_t result = 0;

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
  std::vector<std::size_t> keyTests{99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};

  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    std::vector<testType>                     objectsCopy;
    fetch::random::LaggedFibonacciGenerator<> lfg;

    // Create vector of random numbers
    for (std::size_t i = 0; i < numberOfKeys; ++i)
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

    for (std::size_t i = 0; i < 100; ++i)
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
  std::vector<std::size_t> keyTests{99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};

  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    // Create vector of random numbers
    for (std::size_t i = 0; i < numberOfKeys; ++i)
    {
      testType test;
      test.first  = int(-i);
      test.second = i;

      test.third = std::to_string(i);

      testStore.Set(ResourceAddress(test.third), test);
    }

    bool successfullyFound = false;

    // Expect in the case of hash collisions, we shouldn't find these
    for (std::size_t i = numberOfKeys + 1; i < numberOfKeys * 2; ++i)
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
  std::vector<std::size_t> keyTests{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 99, 100, 1010, 9999};
  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    std::vector<testType>                     objectsCopy;
    fetch::random::LaggedFibonacciGenerator<> lfg;

    // Create vector of random numbers
    for (std::size_t i = 0; i < numberOfKeys; ++i)
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

TEST(storage_object_store_with_STL_gtest, subtree_iterator_over_basic_struct)
{
  std::vector<std::size_t> keyTests{9,  1,  2,  3,  4, 5, 6, 7,  8,  9,   10,  11,
                                    12, 13, 14, 99, 0, 1, 9, 12, 14, 100, 1000};
  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    std::vector<testType>                     objectsCopy;
    fetch::random::LaggedFibonacciGenerator<> lfg;
    testType                                  dummy;

    ByteArray array;
    array.Resize(256 / 8);

    for (std::size_t i = 0; i < array.size(); ++i)
    {
      array[i] = 0;
    }

    // Create vector of random numbers
    for (std::size_t i = 0; i < numberOfKeys; ++i)
    {
      uint64_t random = lfg();

      testType test;
      test.first  = int(-random);
      test.second = random;

      test.third = std::to_string(random);

      testStore.Set(ResourceAddress(test.third), test);
      objects.push_back(test);
    }

    constexpr uint8_t bits{4};
    constexpr uint8_t max_val{1 << bits};
    // Now, aim to split the store up and copy it across perfectly
    for (uint8_t keyBegin = 0; keyBegin < max_val; ++keyBegin)
    {
      array[0] = static_cast<uint8_t>(keyBegin);

      auto rid = ResourceID(array);

      testStore.Get(rid, dummy);

      auto it = testStore.GetSubtree(rid, uint64_t{bits});

      while (it != testStore.end())
      {
        objectsCopy.push_back(*it);
        ++it;
      }
    }

    // expect iterator test to go well
    EXPECT_EQ(objectsCopy.size(), objects.size());

    std::sort(objects.begin(), objects.end());
    std::sort(objectsCopy.begin(), objectsCopy.end());

    bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
    EXPECT_TRUE(allMatch);
  }
}

TEST(storage_object_store_with_STL_gtest, subtree_iterator_over_basic_struc_split_into_256)
{
  std::vector<std::size_t> keyTests{23, 100, 1,  2,  3,   4, 5, 6, 7,  8,  9,   10,  11,
                                    12, 13,  14, 99, 999, 0, 1, 9, 12, 14, 100, 1000};
  for (auto const &numberOfKeys : keyTests)
  {
    using testType = TestSerDeser;
    ObjectStore<testType> testStore;
    testStore.New("testFile.db", "testIndex.db");

    std::vector<testType>                     objects;
    std::vector<testType>                     objectsCopy;
    fetch::random::LaggedFibonacciGenerator<> lfg;

    ByteArray array;
    array.Resize(256 / 8);

    for (std::size_t i = 0; i < array.size(); ++i)
    {
      array[i] = 0;
    }

    testType dummy;

    // Create vector of random numbers
    for (std::size_t i = 0; i < numberOfKeys; ++i)
    {
      uint64_t random = lfg();

      testType test;
      test.first  = int(-random);
      test.second = random;

      test.third = std::to_string(random);

      testStore.Set(ResourceAddress(test.third), test);
      objects.push_back(test);
    }

    // Now, aim to split the store up and copy it across perfectly
    for (uint8_t keyBegin = 0;; ++keyBegin)
    {
      array[0] = (keyBegin);

      auto rid = ResourceID(array);

      testStore.Get(rid, dummy);

      auto it = testStore.GetSubtree(rid, uint64_t(8));

      while (it != testStore.end())
      {
        objectsCopy.push_back(*it);
        ++it;
      }

      if (keyBegin == 0xFF)
      {
        break;
      }
    }

    // expect iterator test to go well
    EXPECT_EQ(objectsCopy.size(), objects.size());

    std::sort(objects.begin(), objects.end());
    std::sort(objectsCopy.begin(), objectsCopy.end());

    bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
    EXPECT_EQ(allMatch, true);
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
