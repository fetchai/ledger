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

#include "crypto/openssl_memory.hpp"

#include <functional>
#include <memory>

#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

class TestType
{
};

class MockDeleterPrimitive
{
public:
  MOCK_CONST_METHOD1(free_TestType, void(TestType *));
  MOCK_CONST_METHOD1(free_clearing_TestType, void(TestType *));

  using Type      = StrictMock<MockDeleterPrimitive>;
  using SharedPtr = std::shared_ptr<MockDeleterPrimitive::Type>;

  static SharedPtr value;
};

MockDeleterPrimitive::SharedPtr MockDeleterPrimitive::value;

template <typename T, const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
struct StaticMockDeleterPrimitive;

template <>
struct StaticMockDeleterPrimitive<TestType>
{
  static void function(TestType *ptr)
  {
    MockDeleterPrimitive::value->free_TestType(ptr);
  }
};

template <>
struct StaticMockDeleterPrimitive<TestType, eDeleteStrategy::clearing>
{
  static void function(TestType *ptr)
  {
    MockDeleterPrimitive::value->free_clearing_TestType(ptr);
  }
};

template <typename T, const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
using OpenSSLDeleter_forTesting = OpenSSLDeleter<
    T, P_DeleteStrategy,
    StaticMockDeleterPrimitive<typename std::remove_const<T>::type, P_DeleteStrategy>>;

class OpenSSLDeleterTest : public testing::Test
{
protected:
  MockDeleterPrimitive::SharedPtr &mock_ = MockDeleterPrimitive::value;

  void SetUp() override
  {
    mock_ = std::make_shared<MockDeleterPrimitive::Type>();
  }

  void TearDown() override
  {
    mock_ = MockDeleterPrimitive::SharedPtr();
  }
};

TEST_F(OpenSSLDeleterTest, test_that_DeleterPrimitive_function_is_called_for_CONST_qualified_type)
{
  TestType        testValue;
  const TestType &const_testValue = testValue;

  //* Expectation
  EXPECT_CALL(*mock_, free_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<const TestType> openSSLDeleter;
  openSSLDeleter(&const_testValue);
}

TEST_F(OpenSSLDeleterTest,
       test_that_DeleterPrimitive_function_is_called_for_NON_const_qualified_type)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*mock_, free_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<TestType> openSSLDeleter;
  openSSLDeleter(&testValue);
}

TEST_F(OpenSSLDeleterTest,
       test_that_clearing_DeleterPrimitive_function_is_called_for_CONST_qualified_type)
{
  TestType        testValue;
  const TestType &const_testValue = testValue;

  //* Expectation
  EXPECT_CALL(*mock_, free_clearing_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<const TestType, eDeleteStrategy::clearing> openSSLDeleter;
  openSSLDeleter(&const_testValue);
}

TEST_F(OpenSSLDeleterTest,
       test_that_clearing_DeleterPrimitive_function_is_called_for_NON_const_qualified_type)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*mock_, free_clearing_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<TestType, eDeleteStrategy::clearing> openSSLDeleter;
  openSSLDeleter(&testValue);
}

}  // namespace

}  // namespace detail
}  // namespace memory
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
