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

#include "storage/versioned_random_access_stack.hpp"
#include "core/random/lfg.hpp"

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

TEST(versioned_random_access_stack, basic_functionality)
{
  constexpr uint64_t                        testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  VersionedRandomAccessStack<TestClass>     stack;
  std::vector<TestClass>                    reference;

  stack.New("VRAS_test.db", "VRAS_test_history.db");

  EXPECT_TRUE(stack.is_open());
  EXPECT_TRUE(stack.DirectWrite() == true)
    << "Expected versioned random access stack to be direct write by default";

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

TEST(versioned_random_access_stack, recovering_state)
{
  constexpr uint64_t                        testSize = 1000;
  constexpr uint64_t                        states   = 5;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  VersionedRandomAccessStack<uint16_t>      stack;

  stack.New("VRAS_test.db", "VRAS_test_history.db");

  // Reference is vector of bookmark-state pairs
  std::vector<std::pair<VersionedRandomAccessStack<uint16_t>::bookmark_type,
    std::vector<uint16_t>>> reference;

  for (std::size_t i = 0; i < states; ++i)
  {
    std::vector<uint16_t> numbers;

    if(i != 0)
    {
      // Set state of our reference to the current stack
      numbers = reference[i-1].second;
    }

    for (std::size_t i = 0; i < testSize; ++i)
    {
      uint16_t rnd = uint16_t(lfg());

      // Try to hit all cases
      numbers.push_back(rnd);
      stack.Push(rnd);

      uint16_t switch_state = rnd % 3;

      switch (switch_state)
      {
        // pop
        case 0:
          {
            numbers.pop_back();
            stack.Pop();
          }
        break;

        // set
        case 1:
          {
            std::size_t set_new = lfg() % numbers.size();
            rnd = uint16_t(lfg());
            numbers[set_new] = rnd;
            stack.Set(set_new, rnd);
          }
        break;

        // swap
        case 2:
          {
            std::size_t swap1 = lfg() % numbers.size();
            std::size_t swap2 = lfg() % numbers.size();

            rnd = numbers[swap1];
            numbers[swap1] = numbers[swap2];
            numbers[swap2] = rnd;

            stack.Swap(swap1, swap2);
          }
        break;
      }
    }
    auto bookmark = stack.Commit();

    reference.push_back(std::make_pair(bookmark, std::move(numbers)));
    stack.Flush();
  }

  // Revert each change in reverse order and verify
  while(reference.size() > 0)
  {
    auto const &pair_ref = reference[reference.size() - 1];

    auto const &bookmark = pair_ref.first;
    auto const &state = pair_ref.second;

    stack.Revert(bookmark);

    // Compare vector to stack
    for (std::size_t i = 0; i < state.size(); ++i)
    {
      ASSERT_TRUE(stack.Get(i) == state[i])
          << "Stack state did not match reference stack at ref size " << reference.size() <<
          " stack position " << i;
    }

    reference.pop_back();
  }

  stack.Clear();
  ASSERT_TRUE(stack.size() == 0);
  ASSERT_TRUE(stack.empty() == true);
}

TEST(versioned_random_access_stack, recovering_state_advanced)
{
  constexpr uint64_t                        testSize = 1000;
  constexpr uint64_t                        states   = 5;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  // Reference is vector of bookmark-state pairs
  std::vector<std::pair<VersionedRandomAccessStack<uint16_t>::bookmark_type,
    std::vector<uint16_t>>> reference;

  // Create random stack
  {
    VersionedRandomAccessStack<uint16_t>      stack;
    stack.New("VRAS_test.db", "VRAS_test_history.db");

    for (std::size_t i = 0; i < states; ++i)
    {
      std::vector<uint16_t> numbers;

      if(i != 0)
      {
        // Set state of our reference to the current stack
        numbers = reference[i-1].second;
      }

      for (std::size_t i = 0; i < testSize; ++i)
      {
        uint16_t rnd = uint16_t(lfg());

        // Try to hit all cases
        numbers.push_back(rnd);
        stack.Push(rnd);

        uint16_t switch_state = rnd % 3;

        switch (switch_state)
        {
          // pop
          case 0:
            {
              numbers.pop_back();
              stack.Pop();
            }
          break;

          // set
          case 1:
            {
              std::size_t set_new = lfg() % numbers.size();
              rnd = uint16_t(lfg());
              numbers[set_new] = rnd;
              stack.Set(set_new, rnd);
            }
          break;

          // swap
          case 2:
            {
              std::size_t swap1 = lfg() % numbers.size();
              std::size_t swap2 = lfg() % numbers.size();

              rnd = numbers[swap1];
              numbers[swap1] = numbers[swap2];
              numbers[swap2] = rnd;

              stack.Swap(swap1, swap2);
            }
          break;
        }
      }
      auto bookmark = stack.Commit();

      reference.push_back(std::make_pair(bookmark, std::move(numbers)));
      stack.Flush();
    }
  }

  // Load stack and check the first bookmark still checks out
  VersionedRandomAccessStack<uint16_t>      stack;
  stack.Load("VRAS_test.db", "VRAS_test_history.db");

  EXPECT_TRUE(stack.is_open());

  {
    auto const &pair_ref = reference[0];

    auto const &bookmark = pair_ref.first;
    auto const &state = pair_ref.second;

    stack.Revert(bookmark);

    // Compare vector to stack
    for (std::size_t i = 0; i < state.size(); ++i)
    {
      ASSERT_TRUE(stack.Get(i) == state[i])
          << "Stack state did not match reference stack at ref size " << reference.size() <<
          " stack position " << i;
    }
  }

  // Check reverting to a non-existant bookmark doesn't break the system
  stack.Revert(999);
}
