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

#include "core/random/lfg.hpp"
#include "storage/variant_stack.hpp"

#include <gtest/gtest.h>
#include <stack>

using namespace fetch::storage;

class TestClass
{
public:
  uint64_t value1 = 0;
  uint8_t  value2 = 0;

  bool operator==(TestClass const &rhs) const
  {
    return value1 == rhs.value1 && value2 == rhs.value2;
  }
};

TEST(variant_stack, basic_functionality)
{
  constexpr uint64_t                                    testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<>             lfg;
  VariantStack                                          stack;
  std::vector<std::tuple<TestClass, uint64_t, uint8_t>> reference;

  stack.New("VS_test.db");

  // Test push/top
  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    std::tuple<TestClass, uint64_t, uint8_t> test_vals =
        std::make_tuple(temp, uint64_t(~random), uint8_t(~random & 0xFF));

    reference.push_back(test_vals);

    uint64_t choose_type = i % 3;

    switch (choose_type)
    {
    case 0:
    {
      stack.Push(std::get<0>(test_vals), choose_type);

      TestClass tmp;
      uint64_t  type = stack.Top(tmp);

      EXPECT_EQ(type, 0) << "Top did not return expected type of 0, returned: " << type;

      ASSERT_EQ(tmp, std::get<0>(test_vals));
    }
    break;
    case 1:
    {
      stack.Push(std::get<1>(test_vals), choose_type);

      uint64_t tmp;
      uint64_t type = stack.Top(tmp);

      EXPECT_EQ(type, 1) << "Top did not return expected type of 1, returned: " << type;

      ASSERT_EQ(tmp, std::get<1>(test_vals));
    }
    break;
    case 2:
    {
      stack.Push(std::get<2>(test_vals), choose_type);

      uint8_t  tmp;
      uint64_t type = stack.Top(tmp);

      EXPECT_EQ(type, 2) << "Top did not return expected type of 2, returned: " << type;

      ASSERT_EQ(tmp, std::get<2>(test_vals));
    }
    break;
    default:
      throw std::out_of_range("failed to index tuple");
    }
  }

  // Pop items off the stack, checking their type
  for (std::size_t i = testSize - 1;; --i)
  {
    uint64_t choose_type = i % 3;

    EXPECT_EQ(choose_type, stack.Type())
        << "Type did not return expected type of " << choose_type << ", returned: " << stack.Type();

    stack.Pop();

    if (i == 0)
    {
      break;
    }
  }

  ASSERT_EQ(stack.size(), 0);
  ASSERT_TRUE(stack.empty());
}

TEST(variant_stack, file_writing_and_recovery)
{
  constexpr uint64_t                                    testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<>             lfg;
  std::vector<std::tuple<TestClass, uint64_t, uint8_t>> reference;

  {
    VariantStack stack;

    stack.New("VS_test_2.db");

    // Fill without tagging by type
    for (uint64_t i = 0; i < testSize; ++i)
    {
      uint64_t  random = lfg();
      TestClass temp;
      temp.value1 = random + 1;
      temp.value2 = random & 0xFF;

      std::tuple<TestClass, uint64_t, uint8_t> test_vals =
          std::make_tuple(temp, uint64_t(~random), uint8_t(~random & 0xFF));

      reference.push_back(test_vals);

      uint64_t choose_type = i % 3;

      if (choose_type == 0)
      {
        stack.Push(std::get<0>(test_vals));
      }

      if (choose_type == 1)
      {
        stack.Push(std::get<1>(test_vals));
      }

      if (choose_type == 2)
      {
        stack.Push(std::get<2>(test_vals));
      }
    }

    stack.Close();
  }

  // Check values against loaded file
  {
    VariantStack stack;

    stack.Load("VS_test_2.db");

    {
      ASSERT_EQ(stack.size(), reference.size());

      for (uint64_t i = testSize - 1;; --i)
      {
        std::tuple<TestClass, uint64_t, uint8_t> test_vals = reference[i];

        uint64_t choose_type = i % 3;

        if (choose_type == 0)
        {
          TestClass tmp;
          stack.Top(tmp);

          ASSERT_EQ(tmp, std::get<0>(test_vals));
        }

        if (choose_type == 1)
        {
          uint64_t tmp;
          stack.Top(tmp);

          ASSERT_EQ(tmp, std::get<1>(test_vals));
        }

        if (choose_type == 2)
        {
          uint8_t tmp;
          stack.Top(tmp);

          ASSERT_EQ(tmp, std::get<2>(test_vals));
        }

        stack.Pop();

        if (i == 0)
        {
          break;
        }
      }
    }

    stack.Close();
  }

  // Check we can clear and set new elements after loading
  {
    VariantStack stack;

    stack.Load("VS_test_2.db");

    reference.clear();
    stack.Clear();

    ASSERT_EQ(stack.size(), reference.size());

    ASSERT_EQ(stack.size(), 0);
    ASSERT_TRUE(stack.empty());

    // push types onto variant stack
    for (uint64_t i = 0; i < testSize; ++i)
    {
      uint64_t  random = lfg();
      TestClass temp;
      temp.value1 = random;
      temp.value2 = random & 0xFF;

      std::tuple<TestClass, uint64_t, uint8_t> test_vals =
          std::make_tuple(temp, uint64_t(~random), uint8_t(~random & 0xFF));

      reference.push_back(test_vals);

      uint64_t choose_type = i % 3;

      switch (choose_type)
      {
      case 0:
      {
        stack.Push(std::get<0>(test_vals), choose_type);

        TestClass tmp;
        uint64_t  type = stack.Top(tmp);

        EXPECT_EQ(type, 0) << "Top did not return expected type of 0, returned: " << type;

        ASSERT_EQ(tmp, std::get<0>(test_vals));
      }
      break;
      case 1:
      {
        stack.Push(std::get<1>(test_vals), choose_type);

        uint64_t tmp;
        uint64_t type = stack.Top(tmp);

        EXPECT_EQ(type, 1) << "Top did not return expected type of 1, returned: " << type;

        ASSERT_EQ(tmp, std::get<1>(test_vals));
      }
      break;
      case 2:
      {
        stack.Push(std::get<2>(test_vals), choose_type);

        uint8_t  tmp;
        uint64_t type = stack.Top(tmp);

        EXPECT_EQ(type, 2) << "Top did not return expected type of 2, returned: " << type;

        ASSERT_EQ(tmp, std::get<2>(test_vals));
      }
      break;
      default:
        throw std::out_of_range("failed to index tuple");
      }
    }

    // Pop items off the stack, checking their type
    for (std::size_t i = testSize - 1;; --i)
    {
      uint64_t choose_type = i % 3;

      EXPECT_EQ(choose_type, stack.Type()) << "Type did not return expected type of " << choose_type
                                           << ", returned: " << stack.Type();

      stack.Pop();

      if (i == 0)
      {
        break;
      }
    }
  }
}
