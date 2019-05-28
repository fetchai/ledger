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
#include "storage/random_access_stack.hpp"

#include <gmock/gmock.h>

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

class MockStream
{
public:
  MockStream()
  {}

  MockStream(std::string /*a*/, std::ios_base::openmode /*b*/)
  {}

  operator bool() const
  {
    return true;
  }

  MockStream &operator=(const MockStream & /*other*/)
  {
    return *this;
  }

  bool is_open()
  {
    return true;
  }

  MOCK_CONST_METHOD0(is_open, bool());
  MOCK_METHOD0(close, void());
  MOCK_METHOD2(read, MockStream &(char *, std::streamsize));
  MOCK_METHOD2(write, MockStream &(const char *, std::streamsize));
  MOCK_METHOD2(seekg, MockStream &(std::streamoff, std::ios_base::seekdir));
  MOCK_METHOD1(seekg, MockStream &(std::streampos));
  MOCK_METHOD0(tellg, std::streampos());
  MOCK_METHOD2(open, void(std::string, std::ios_base::openmode));

  const std::ios_base::seekdir beg = std::ios_base::beg;
  const std::ios_base::seekdir end = std::ios_base::end;
};

// fails due to badly mocking the is_open functionality
TEST(DISABLED_random_access_stack, mocked_test_get_set)
{
  RandomAccessStack<TestClass, uint64_t, MockStream> stack;

  std::shared_ptr<MockStream> mocked = std::make_shared<MockStream>();
  MockStream                  dummy;
  EXPECT_CALL(stack.underlying_stream(), is_open()).Times(1).WillRepeatedly(testing::Return(true));
  EXPECT_CALL(stack.underlying_stream(), close()).Times(1);

  EXPECT_CALL(stack.underlying_stream(), seekg(testing::_))
      .Times(1)
      .WillRepeatedly(testing::ReturnRef(dummy));
  EXPECT_CALL(stack.underlying_stream(), seekg(testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(testing::ReturnRef(dummy));
  EXPECT_CALL(stack.underlying_stream(), read(testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(testing::ReturnRef(dummy));
  EXPECT_CALL(stack.underlying_stream(), write(testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(testing::ReturnRef(dummy));

  TestClass temp;
  stack.New("abcd");
  stack.Set(0, temp);
  stack.Get(0, temp);
}

// fails due to badly mocking the is_open functionality
TEST(DISABLED_random_access_stack, mocked_test_load)
{
  RandomAccessStack<TestClass, uint64_t, MockStream> stack;

  EXPECT_CALL(stack.underlying_stream(), is_open())
      .Times(2)
      .WillOnce(testing::Return(false))
      .WillOnce(testing::Return(true));
  EXPECT_CALL(stack.underlying_stream(), close()).Times(1);
  // EXPECT_CALL(stack.underlying_stream(), seekg(testing::_, testing::_)) .Times(2)
  // .WillRepeatedly(testing::ReturnRef(dummy));
  EXPECT_CALL(stack.underlying_stream(), tellg()).Times(1).WillRepeatedly(testing::Return(10));

  stack.Load("abce");
}

TEST(random_access_stack, basic_functionality)
{
  constexpr uint64_t                        testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  RandomAccessStack<TestClass>              stack;
  std::vector<TestClass>                    reference;

  stack.New("RAS_test.db");

  EXPECT_TRUE(stack.is_open());
  EXPECT_TRUE(stack.DirectWrite()) << "Expected random access stack to be direct write";

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

    ASSERT_EQ(stack.Top(), reference[i]) << "Stack did not match reference stack at index " << i;
  }

  // Test index
  {
    ASSERT_EQ(stack.size(), reference.size());

    for (uint64_t i = 0; i < testSize; ++i)
    {
      TestClass temp;
      stack.Get(i, temp);
      ASSERT_EQ(temp, reference[i]);
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

      ASSERT_EQ(c, b) << "Stack swap test failed, iteration " << i;
    }

    {
      TestClass c;
      stack.Get(pos2, c);

      ASSERT_EQ(c, a) << "Stack swap test failed, iteration " << i;
    }
  }

  // Pop items off the stack and check size
  for (std::size_t i = 0; i < testSize; ++i)
  {
    stack.Pop();
    reference.pop_back();
    ASSERT_EQ(stack.size(), reference.size());
  }

  ASSERT_EQ(stack.size(), 0);
  ASSERT_TRUE(stack.empty());
}

TEST(random_access_stack, file_writing_and_recovery)
{
  constexpr uint64_t                        testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  std::vector<TestClass>                    reference;

  {
    RandomAccessStack<TestClass> stack;

    // Testing closures
    bool file_loaded  = false;
    bool file_flushed = false;

    stack.OnFileLoaded([&file_loaded] { file_loaded = true; });
    stack.OnBeforeFlush([&file_flushed] { file_flushed = true; });

    stack.New("RAS_test_2.db");

    EXPECT_TRUE(file_loaded);

    stack.SetExtraHeader(0x00deadbeefcafe00);
    EXPECT_EQ(stack.header_extra(), 0x00deadbeefcafe00);

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

    stack.Flush();
    EXPECT_TRUE(file_flushed);
  }

  // Check values against loaded file
  {
    RandomAccessStack<TestClass> stack;

    stack.Load("RAS_test_2.db");

    EXPECT_EQ(stack.header_extra(), 0x00deadbeefcafe00);

    {
      ASSERT_EQ(stack.size(), reference.size());

      for (uint64_t i = 0; i < testSize; ++i)
      {
        TestClass temp;
        stack.Get(i, temp);
        ASSERT_EQ(temp, reference[i]);
      }
    }

    stack.Close();
  }
}

TEST(random_access_stack, bulk_functionality)
{
  constexpr uint64_t                        testSize = 10000;
  fetch::random::LaggedFibonacciGenerator<> lfg;
  RandomAccessStack<TestClass>              stack;
  TestClass                                 referenceSet[testSize];
  TestClass                                 referenceGet[testSize];

  stack.New("RAS_test.db");

  EXPECT_TRUE(stack.is_open());
  EXPECT_TRUE(stack.DirectWrite()) << "Expected random access stack to be direct write";

  for (uint64_t i = 0; i < testSize; i++)
  {
    uint64_t  random = lfg();
    TestClass temp;
    temp.value1 = random;
    temp.value2 = random & 0xFF;

    referenceSet[i] = temp;
  }

  stack.SetBulk(0, testSize, referenceSet);
  stack.GetBulk(0, testSize, referenceGet);

  for (uint64_t i = 0; i < testSize; i++)
  {
    EXPECT_EQ(referenceSet[i], referenceGet[i]);
  }
}
