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
#include "storage/versioned_random_access_stack.hpp"

#include "gtest/gtest.h"

#include <vector>

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

#define TYPE uint64_t

TEST(versioned_random_access_stack_gtest, creation_and_manipulation)
{
  VersionedRandomAccessStack<TYPE> stack;
  stack.New("versioned_random_access_stack_test_1.db", "versioned_random_access_stack_diff.db");
  VersionedRandomAccessStack<TYPE>::bookmark_type cp1, cp2, cp3;

  // testing basic manipulation)
  cp1 = stack.Commit();
  stack.Push(1);
  stack.Push(2);
  stack.Push(3);
  cp2 = stack.Commit();
  stack.Swap(1, 2);
  stack.Push(4);
  stack.Push(5);
  stack.Set(0, 9);
  cp3 = stack.Commit();
  stack.Push(6);
  stack.Push(7);
  stack.Push(9);
  stack.Pop();
  EXPECT_EQ(stack.Top(), 7);
  EXPECT_EQ(stack.Get(0), 9);
  EXPECT_EQ(stack.Get(1), 3);
  EXPECT_EQ(stack.Get(2), 2);

  // testing revert 1
  stack.Revert(cp3);
  EXPECT_EQ(stack.Top(), 5);
  EXPECT_EQ(stack.Get(0), 9);
  EXPECT_EQ(stack.Get(1), 3);

  EXPECT_EQ(stack.Get(2), 2);

  // testing revert 2
  stack.Revert(cp2);
  EXPECT_EQ(stack.Top(), 3);
  EXPECT_EQ(stack.Get(0), 1);
  EXPECT_EQ(stack.Get(1), 2);
  EXPECT_EQ(stack.Get(2), 3);

  // testing revert 3
  stack.Revert(cp1);
  EXPECT_NE(stack.empty(), 0);

  // testing refilling
  cp1 = stack.Commit();
  stack.Push(1);
  stack.Push(2);
  stack.Push(3);
  cp2 = stack.Commit();
  stack.Swap(1, 2);
  stack.Push(4);
  stack.Push(5);
  stack.Set(0, 9);
  cp3 = stack.Commit();
  stack.Push(6);
  stack.Push(7);
  stack.Push(9);
  stack.Pop();

  EXPECT_EQ(stack.Top(), 7);
  EXPECT_EQ(stack.Get(0), 9);
  EXPECT_EQ(stack.Get(1), 3);
  EXPECT_EQ(stack.Get(2), 2);

  // testing revert 2
  stack.Revert(cp2);
  EXPECT_EQ(stack.Top(), 3);
  EXPECT_EQ(stack.Get(0), 1);
  EXPECT_EQ(stack.Get(1), 2);
  EXPECT_EQ(stack.Get(2), 3);
}

TEST(versioned_random_access_stack_gtest, storage_of_large_objects)
{
  fetch::random::LaggedFibonacciGenerator<> lfg;

  struct Element
  {
    int      a;
    uint8_t  b;
    uint64_t c;
    uint16_t d;

    bool operator==(Element const &o) const
    {
      return ((a == o.a) && (b == o.b) && (c == o.c) && (d == o.d));
    }
  };
  VersionedRandomAccessStack<Element> stack;
  stack.New("versioned_random_access_stack_test_2.db", "versioned_random_access_stack_diff2.db");
  std::vector<Element> reference;

  auto newElement = [&stack, &reference, &lfg]() -> Element {
    Element e;
    e.a = int(lfg());
    e.b = uint8_t(lfg());
    e.c = uint64_t(lfg());
    e.d = uint16_t(lfg());
    stack.Push(e);
    reference.push_back(e);
    return e;
  };

  bool all_equal = true;
  for (std::size_t i = 1; i < 20; ++i)
  {
    if ((i % 4) == 0)
    {
      stack.Commit();
    }
    newElement();
    all_equal &= (stack.Top() == reference.back());
  }
  EXPECT_TRUE(all_equal);

  all_equal = true;
  for (std::size_t i = 0; i < reference.size(); ++i)
  {
    all_equal &= (stack.Get(i) == reference[i]);
  }
  EXPECT_TRUE(all_equal);
}
