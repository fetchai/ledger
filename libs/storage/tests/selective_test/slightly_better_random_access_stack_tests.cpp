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

#include "core/random/lfg.hpp"
#include "storage/slightly_better_random_access_stack.hpp"

#include <gtest/gtest.h>
#include <stack>

using namespace fetch::storage;

class TestClass
{
public:
  uint64_t value1 = 0;
  uint8_t  value2 = 0;

  bool operator==(TestClass const &rhs)
  {
    return value1 == rhs.value1 && value2 == rhs.value2;
  }
};

TEST(slightly_better_random_access_stack, basic_functionality)
{
  constexpr uint64_t                         testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<>  lfg;
  SlightlyBetterRandomAccessStack<TestClass> stack;
  std::vector<TestClass>                     reference;

  stack.New("CRAS_test.db");

  EXPECT_TRUE(stack.is_open());

  // Test push/top
  for (uint64_t i = 0; i < testSize; ++i)
  {
    {
      uint64_t  random = lfg();
      TestClass temp;
      temp.value1 = random;
      temp.value2 = random & 0xFF;

      stack.Push(temp);
      reference.push_back(temp);
    }

    ASSERT_TRUE(stack.Top() == reference[i])
        << "Stack did not match reference stack at index " << i;
  }

  // Test index
  {
    ASSERT_TRUE(stack.size() == reference.size());

    for (uint64_t i = 0; i < testSize; ++i)
    {
      TestClass temp;
      stack.Get(i, temp);
      ASSERT_TRUE(temp == reference[i]);
    }
  }

  // Test setting
  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    stack.Set(i, temp);
    reference[i] = temp;
  }

  // Test swapping
  for (std::size_t i = 0; i < 100; ++i)
  {
    uint64_t pos1 = lfg() % testSize;
    uint64_t pos2 = lfg() % testSize;

    TestClass a;
    stack.Get(pos1, a);

    TestClass b;
    stack.Get(pos2, b);

    stack.Swap(pos1, pos2);

    {
      TestClass c;
      stack.Get(pos1, c);

      ASSERT_TRUE(c == b) << "Stack swap test failed, iteration " << i;
    }

    {
      TestClass c;
      stack.Get(pos2, c);

      ASSERT_TRUE(c == a) << "Stack swap test failed, iteration " << i;
    }
  }

  // Pop items off the stack
  for (std::size_t i = 0; i < testSize; ++i)
  {
    stack.Pop();
  }

  ASSERT_TRUE(stack.size() == 0);
  ASSERT_TRUE(stack.empty() == true);
}

TEST(slightly_better_random_access_stack, file_writing_and_recovery)
{
  constexpr uint64_t                        testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  std::vector<TestClass>                    reference;

  {
    SlightlyBetterRandomAccessStack<TestClass> stack;

    stack.New("CRAS_test_2.db");

    stack.SetExtraHeader(0x00deadbeefcafe00);
    EXPECT_TRUE(stack.header_extra() == 0x00deadbeefcafe00);

    // Fill with random numbers
    for (uint64_t i = 0; i < testSize; ++i)
    {
      uint64_t  random = lfg();
      TestClass temp;
      temp.value1 = random;
      temp.value2 = random & 0xFF;

      stack.Push(temp);
      reference.push_back(temp);
    }
  }

  // Check values against loaded file
  {
    SlightlyBetterRandomAccessStack<TestClass> stack;

    stack.Load("CRAS_test_2.db");

    EXPECT_TRUE(stack.header_extra() == 0x00deadbeefcafe00);

    {
      ASSERT_TRUE(stack.size() == reference.size());

      for (uint64_t i = 0; i < testSize; ++i)
      {
        TestClass temp;

        stack.Get(i, temp);

        ASSERT_TRUE(temp == reference[i]);
      }
    }

    stack.Close();
  }

  // Check we can set new elements after loading
  {
    SlightlyBetterRandomAccessStack<TestClass> stack;

    stack.Load("CRAS_test_2.db");

    EXPECT_TRUE(stack.header_extra() == 0x00deadbeefcafe00);

    {
      ASSERT_TRUE(stack.size() == reference.size());

      for (uint64_t i = 0; i < testSize; ++i)
      {
        TestClass temp;
        temp.value1 = i;
        temp.value2 = i & 0xFF;

        stack.Set(i, temp);
        reference[i] = temp;
      }
    }

    stack.Close();
  }

  // Verify
  {
    SlightlyBetterRandomAccessStack<TestClass> stack;

    stack.Load("CRAS_test_2.db");

    {
      ASSERT_TRUE(stack.size() == reference.size());

      for (uint64_t i = 0; i < testSize; ++i)
      {
        TestClass temp;
        stack.Get(i, temp);
        EXPECT_TRUE(temp == reference[i]);
      }
    }

    stack.Close();
  }
}
