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

#include "crypto/openssl_context_session.hpp"

#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

struct TestType
{
};

class MockContextPrimitive
{
public:
  MOCK_CONST_METHOD1(start, void(TestType *));
  MOCK_CONST_METHOD1(end, void(TestType *));

  using Type      = StrictMock<MockContextPrimitive>;
  using SharedPtr = std::shared_ptr<MockContextPrimitive::Type>;

  static SharedPtr value;
};

MockContextPrimitive::SharedPtr MockContextPrimitive::value;

template <typename T>
struct StaticMockContextPrimitive;

template <>
struct StaticMockContextPrimitive<TestType>
{
  static void start(TestType *ptr)
  {
    MockContextPrimitive::value->start(ptr);
  }
  static void end(TestType *ptr)
  {
    MockContextPrimitive::value->end(ptr);
  }
};

class Deleter
{
public:
  void operator()(TestType *)  // NOLINT
  {}
};

class OpenSSLContextSessionTest : public testing::Test
{
protected:
  template <const memory::eDeleteStrategy P_DeleteStrategy = memory::eDeleteStrategy::canonical>
  using ossl_shared_ptr__for_Testing = memory::OsslSharedPtr<TestType, P_DeleteStrategy, Deleter>;
  using Session__for_Testing =
      Session<TestType, StaticMockContextPrimitive<TestType>, ossl_shared_ptr__for_Testing<>>;

  MockContextPrimitive::SharedPtr &contextMock_ = MockContextPrimitive::value;

  void SetUp() override
  {
    contextMock_ = std::make_shared<MockContextPrimitive::Type>();
  }

  void TearDown() override
  {
    contextMock_ = MockContextPrimitive::SharedPtr();
  }
};

TEST_F(OpenSSLContextSessionTest, test_Session_basic_scenario_constructro_and_destructor)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*contextMock_, start(&testValue)).WillOnce(Return());
  EXPECT_CALL(*contextMock_, end(&testValue)).WillOnce(Return());

  {
    //* Production code
    ossl_shared_ptr__for_Testing<> x(&testValue);

    Session__for_Testing session(x);
  }
}

TEST_F(OpenSSLContextSessionTest, test_Session_constructor_and_end)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*contextMock_, start(&testValue)).WillOnce(Return());
  EXPECT_CALL(*contextMock_, end(&testValue)).WillOnce(Return());

  {
    //* Production code
    ossl_shared_ptr__for_Testing<> x(&testValue);

    Session__for_Testing session(x);
    session.end();
  }
}

TEST_F(OpenSSLContextSessionTest, test_Session_started_and_destructor)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*contextMock_, end(&testValue)).WillOnce(Return());

  {
    //* Production code
    ossl_shared_ptr__for_Testing<> x(&testValue);

    Session__for_Testing session(x, true);
  }
}

TEST_F(OpenSSLContextSessionTest, test_Session_started_and_end)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*contextMock_, end(&testValue)).WillOnce(Return());

  {
    //* Production code
    ossl_shared_ptr__for_Testing<> x(&testValue);

    Session__for_Testing session(x, true);
    session.end();
  }
}

TEST_F(OpenSSLContextSessionTest, test_Session_constructor_and_start_and_destructor)
{
  TestType testValue;

  //* Expectation
  EXPECT_CALL(*contextMock_, start(&testValue)).WillOnce(Return());
  EXPECT_CALL(*contextMock_, end(&testValue)).WillOnce(Return());

  {
    //* Production code
    ossl_shared_ptr__for_Testing<> x(&testValue);

    Session__for_Testing session(x);
    session.start();
  }
}

}  // namespace

}  // namespace context
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
