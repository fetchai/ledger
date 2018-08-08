#include "crypto/openssl_memory.hpp"

#include <functional>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

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
  static void function(TestType *ptr) { MockDeleterPrimitive::value->free_TestType(ptr); }
};

template <>
struct StaticMockDeleterPrimitive<TestType, eDeleteStrategy::clearing>
{
  static void function(TestType *ptr) { MockDeleterPrimitive::value->free_clearing_TestType(ptr); }
};

template <typename T, const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
using OpenSSLDeleter_forTesting = OpenSSLDeleter<
    T, P_DeleteStrategy,
    StaticMockDeleterPrimitive<typename std::remove_const<T>::type, P_DeleteStrategy>>;

class OpenSSLDeleterTest : public testing::Test
{
protected:
  MockDeleterPrimitive::SharedPtr &mock = MockDeleterPrimitive::value;

  void SetUp() override { mock = std::make_shared<MockDeleterPrimitive::Type>(); }

  void TearDown() override { mock = MockDeleterPrimitive::SharedPtr(); }

  // static void SetUpTestCase() {
  //}

  // static void TearDownTestCase() {
  //}
};

TEST_F(OpenSSLDeleterTest, test_that_DeleterPrimitive_function_is_called_for_CONST_qualified_type)
{
  TestType        testValue;
  const TestType &const_testValue = testValue;

  //* Expctation
  EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<const TestType> openSSLDeleter;
  openSSLDeleter(&const_testValue);
}

TEST_F(OpenSSLDeleterTest,
       test_that_DeleterPrimitive_function_is_called_for_NON_const_qualified_type)
{
  TestType testValue;

  //* Expctation
  EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<TestType> openSSLDeleter;
  openSSLDeleter(&testValue);
}

TEST_F(OpenSSLDeleterTest,
       test_that_clearing_DeleterPrimitive_function_is_called_for_CONST_qualified_type)
{
  TestType        testValue;
  const TestType &const_testValue = testValue;

  //* Expctation
  EXPECT_CALL(*mock, free_clearing_TestType(&testValue)).WillOnce(Return());

  //* Production code
  OpenSSLDeleter_forTesting<const TestType, eDeleteStrategy::clearing> openSSLDeleter;
  openSSLDeleter(&const_testValue);
}

TEST_F(OpenSSLDeleterTest,
       test_that_clearing_DeleterPrimitive_function_is_called_for_NON_const_qualified_type)
{
  TestType testValue;

  //* Expctation
  EXPECT_CALL(*mock, free_clearing_TestType(&testValue)).WillOnce(Return());

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
