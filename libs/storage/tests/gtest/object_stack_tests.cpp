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
#include "storage/object_stack.hpp"
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
void Serialize(T &serializer, TestSerDeser const &b)
{
  serializer << b.first;
  serializer << b.second;
  serializer << b.third;
}

template <typename T>
void Deserialize(T &serializer, TestSerDeser &b)
{
  serializer >> b.first;
  serializer >> b.second;
  serializer >> b.third;
}

void CheckIdentical(ObjectStack<TestSerDeser> &      test_stack,
                    std::vector<TestSerDeser> const &ref_stack)
{
  EXPECT_EQ(test_stack.size(), ref_stack.size());

  for (std::size_t i = 0; i < ref_stack.size(); ++i)
  {
    TestSerDeser getme;
    test_stack.Get(i, getme);

    EXPECT_EQ(ref_stack[i], getme);
  }
};

TEST(storage_object_stack_basic_functionality, pushing_and_popping)
{
  fetch::random::LaggedFibonacciGenerator<> lfg;
  ObjectStack<TestSerDeser>                 test_stack;
  std::vector<TestSerDeser>                 ref_stack;

  test_stack.New("a.db", "b.db");

  for (int i = 0; i < 100; ++i)
  {
    TestSerDeser item{i, uint64_t(i + 1), std::to_string(i)};
    test_stack.Push(item);
    ref_stack.push_back(item);

    CheckIdentical(test_stack, ref_stack);

    if (lfg() & 0x1)
    {
      test_stack.Pop();
      ref_stack.pop_back();
      CheckIdentical(test_stack, ref_stack);
    }
  }
}
