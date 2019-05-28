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
#include "storage/cached_random_access_stack.hpp"

#include <gmock/gmock.h>

using namespace fetch::storage;
using namespace testing;

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
template <typename elem>
class MockStack
{

public:
  MockStack()
  {}
  MockStack(const MockStack & /*mock_obj*/)
  {}
  ~MockStack()
  {}
  MOCK_CONST_METHOD0_T(is_open, bool());
  MOCK_METHOD0_T(Clear, void());
  MOCK_METHOD1_T(Flush, void(bool));
  MOCK_METHOD1_T(New, void(std::string));
  MOCK_METHOD2_T(Load, void(std::string, bool));
  MOCK_METHOD0_T(ClearEventHandlers, void());
  MOCK_METHOD1_T(OnFileLoaded, bool(std::function<void()>));
  MOCK_METHOD1_T(OnBeforeFlush, void(std::function<void()>));
  MOCK_METHOD1_T(Push, void(elem const));
  MOCK_METHOD1_T(LazyPush, uint64_t(elem const));
  MOCK_CONST_METHOD2_T(Get, void(std::size_t const, elem &));
  MOCK_CONST_METHOD2_T(Set, void(std::size_t const, elem const &));
  MOCK_CONST_METHOD0_T(size, std::size_t());
  MOCK_CONST_METHOD1_T(SetExtraHeader, void(uint64_t));
  MOCK_METHOD1_T(Close, void(bool const));
  bool TestingFunc()
  {
    return true;
  }
};

TEST(mocked_cached_random_access_stack, new_stack)
{
  using MockStackCurrent = testing::NiceMock<MockStack<TestClass>>;

  CachedRandomAccessStack<TestClass, uint64_t, MockStackCurrent> cached_stack;

  EXPECT_CALL(cached_stack.underlying_stack(), New("testFile.db")).Times(1);
  EXPECT_CALL(cached_stack.underlying_stack(), Clear()).Times(1);
  /* EXPECT_CALL(cached_stack.underlying_stack(), is_open()).WillOnce(Return(true)); */

  cached_stack.New("testFile.db");

  /* EXPECT_TRUE(cached_stack.is_open()); */
  EXPECT_FALSE(cached_stack.DirectWrite()) << "Expected cached random access stack to be caching";
}

TEST(mocked_cached_random_access_stack, file_writing_and_closing)
{
  using MockStackCurrent = testing::NiceMock<MockStack<TestClass>>;

  CachedRandomAccessStack<TestClass, uint64_t, MockStackCurrent> cached_stack;
  fetch::random::LaggedFibonacciGenerator<>                      lfg;
  constexpr uint64_t                                             testSize = 10000;
  uint64_t                                                       temp_val = 0;

  bool file_flushed = false;

  cached_stack.OnBeforeFlush([&file_flushed] { file_flushed = true; });

  EXPECT_CALL(cached_stack.underlying_stack(), SetExtraHeader(0x00deadbeefcafe00))
      .WillOnce(testing::Return());
  EXPECT_CALL(cached_stack.underlying_stack(), LazyPush(testing::_))
      .WillRepeatedly(testing::Return(temp_val));
  /* EXPECT_CALL(cached_stack.underlying_stack(), Set(testing::_,
   * testing::_)).Times(1).WillRepeatedly(testing::Return()); */

  /* EXPECT_CALL(cached_stack.underlying_stack(), Flush(true)).Times(2); */
  /* EXPECT_CALL(cached_stack.underlying_stack(), Close(true)).Times(1); */

  cached_stack.New("abcnew");

  cached_stack.SetExtraHeader(0x00deadbeefcafe00);

  for (uint64_t i = 0; i < testSize; ++i)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    /* cached_stack.Push(temp); */
  }

  cached_stack.Flush();
  EXPECT_TRUE(file_flushed);
  cached_stack.Close();
}

TEST(mocked_cached_random_access_stack, push_top_pop_elements)
{
  using MockStackCurrent = testing::NiceMock<MockStack<TestClass>>;

  CachedRandomAccessStack<TestClass, uint64_t, MockStackCurrent> cached_stack;
  fetch::random::LaggedFibonacciGenerator<>                      lfg;

  // never does get since its caching
  /* EXPECT_CALL(cached_stack.underlying_stack(), Get(testing::_, testing::_)); */

  cached_stack.New("abcrandom");

  uint64_t random = lfg();
  uint64_t obj_index;

  // Testing Push and Top
  TestClass temp;
  temp.value1 = random;
  temp.value2 = random & 0xFF;

  obj_index = cached_stack.Push(temp);
  EXPECT_EQ(cached_stack.Top(), temp);

  // Testing Get when object is in cache
  TestClass temp_obj;
  cached_stack.Get(obj_index, temp_obj);
  EXPECT_EQ(temp, temp_obj);

  // Testing get when index is greater than stack-size .It should return with no object
  /* uint64_t index = cached_stack.size(); */
  /* cached_stack.Get(index, temp_obj); */

  // Testing Pop
  cached_stack.Pop();
  EXPECT_EQ(cached_stack.size(), 0);  // After pop of last element size will be zero
}

TEST(mocked_cached_random_access_stack, get_set_swap_elements)
{
  using MockStackCurrent = testing::NiceMock<MockStack<TestClass>>;

  CachedRandomAccessStack<TestClass, uint64_t, MockStackCurrent> cached_stack;
  fetch::random::LaggedFibonacciGenerator<>                      lfg;

  cached_stack.New("abcnewest");

  TestClass write_obj1, write_obj2;
  write_obj1.value1 = lfg();
  write_obj1.value2 = write_obj1.value1 & 0xFF;
  write_obj2.value1 = lfg();
  write_obj2.value2 = write_obj2.value1 & 0xFF;

  // underlying stack is mocked for now
  /*cached_stack.Set(0, write_obj1);
  cached_stack.Set(1, write_obj2);

  cached_stack.Swap(0, 1);

  TestClass read_obj1, read_obj2;
  cached_stack.Get(0, read_obj1);
  cached_stack.Get(1, read_obj2);

  EXPECT_EQ(write_obj2, read_obj1);
  EXPECT_EQ(write_obj1, read_obj2); */
}

TEST(mocked_cached_random_access_stack, file_loading_and_closing)
{
  using MockStackCurrent = testing::NiceMock<MockStack<TestClass>>;

  CachedRandomAccessStack<TestClass, uint64_t, MockStackCurrent> cached_stack;
  fetch::random::LaggedFibonacciGenerator<>                      lfg;

  std::string file_name{"abcthing"};

  EXPECT_CALL(cached_stack.underlying_stack(), Load(file_name, true)).Times(1);
  EXPECT_CALL(cached_stack.underlying_stack(), Close(true)).Times(1);

  bool file_loaded = false;
  cached_stack.OnFileLoaded([&file_loaded] { file_loaded = true; });

  cached_stack.Load(file_name, true);
  cached_stack.Close();
}
