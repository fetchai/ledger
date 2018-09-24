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

#include "storage/object_store.hpp"
#include "core/random/lfg.hpp"

#include <gtest/gtest.h>

using namespace fetch::storage;

struct TestSerDeser
{
  int         first;
  uint64_t    second;
  std::string third;

  bool operator<(TestSerDeser const &rhs) const
  {
    return third < rhs.third;
  }

  bool operator==(TestSerDeser const &rhs) const
  {
    return first == rhs.first && second == rhs.second && third == rhs.third;
  }
};

template <typename T>
inline void Serialize(T &serializer, TestSerDeser const &b)
{
  serializer << b.first;
  serializer << b.second;
  serializer << b.third;
}

template <typename T>
inline void Deserialize(T &serializer, TestSerDeser &b)
{
  serializer >> b.first;
  serializer >> b.second;
  std::string ret;
  serializer >> ret;
  b.third = ret;
}

class ObjectStoreTest : public ::testing::Test {

protected:
  using testType = TestSerDeser;

  // GTest setup
  void SetUp() override
  {
  }

  // custom setup - populate our object store and a vector with random elements
  void SetUp(uint64_t test_size)
  {
    test_store_.New("testFile.db", "testIndex.db");
    test_store_small_.New("testFile2.db", "testIndex2.db");

    // Populate vector of random numbers
    for (std::size_t i = 0; i < test_size; ++i)
    {
      AddRandomElement();
    }
  }

  void AddRandomElement()
  {
    uint64_t random = lfg_();

    testType test;
    test.first  = int(-random);
    test.second = random;

    test.third = std::to_string(random);

    test_store_.Set(ToAddress(test), test);
    test_store_small_.Set(ToAddress(test), test);
    test_elements_.push_back(test);
  }

  // Verify that test_store_ == test_elements_
  bool Verify()
  {
    valid_ = true;

    Verify(test_store_);
    Verify(test_store_small_);

    return valid_;
  }

  template <typename STORE>
  void Verify(STORE &store)
  {
    if(store.size() != test_elements_.size())
    {
      valid_ = false;
    }

    for(auto const &i : test_elements_)
    {
      testType test;

      if(!store.Get(ToAddress(i), test))
      {
        std::cout << "Failed to find element" << std::endl;
        valid_ = false;
      }

      if(!(test == i))
      {
        std::cout << "Deserialised value not matched" << std::endl;
        valid_ = false;
      }
    }
  }

  ResourceAddress ToAddress(testType const &element) const
  {
    return ResourceAddress(element.third);
  }

  void TearDown() override
  {
    test_elements_.clear();
  }

  bool                                      valid_ = true;
  fetch::random::LaggedFibonacciGenerator<> lfg_;
  ObjectStore<testType>                     test_store_;
  ObjectStore<testType, 5>                  test_store_small_;
  std::vector<testType>                     test_elements_;
};

fetch::random::LaggedFibonacciGenerator<> lfg;

TEST_F(ObjectStoreTest, correct_setup)
{
  SetUp(100);

  ASSERT_TRUE(test_elements_.size() == 100);
  ASSERT_TRUE(Verify() == true);

  test_elements_.pop_back();

  ASSERT_TRUE(Verify() == false);
}

TEST_F(ObjectStoreTest, basic_deletion_of_elements)
{
  std::vector<uint64_t> test_vals = {1,2,3,4,100,1000};

  for(auto const &i : test_vals)
  {
    TearDown();
    SetUp(i);

    // Easiest to remove the last element of the vector
    auto last_element = test_elements_.back();
    test_elements_.pop_back();

    test_store_.Erase(ToAddress(last_element));
    test_store_small_.Erase(ToAddress(last_element));

    ASSERT_TRUE(Verify() == true) << "Failed to delete on iteration: " << i;
  }
}

TEST_F(ObjectStoreTest, advanced_deletion_of_elements)
{
  std::vector<uint64_t> test_vals = {1,2,3,4,100,1000};

  for(auto const &i : test_vals)
  {
    TearDown();
    SetUp(i);

    // Easiest to remove the last element of the vector
    auto last_element = test_elements_.back();
    test_elements_.pop_back();

    test_store_.Erase(ToAddress(last_element));
    test_store_small_.Erase(ToAddress(last_element));

    // Continue to use the stores
    for (std::size_t j = 0; j < i; ++j)
    {
      AddRandomElement();
    }

    ASSERT_TRUE(Verify() == true) << "Failed to delete on iteration: " << i;
  }
}
