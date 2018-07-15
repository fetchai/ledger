#include "crypto/openssl_memory.hpp"

#include <functional>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

struct TestType
{
    void testMethod() {
    }
};

class MockDeleterPrimitive {
public:
    MOCK_CONST_METHOD1(free_TestType, void(TestType*));

    using Type = StrictMock<MockDeleterPrimitive>;
    using SharedPtr = std::shared_ptr<MockDeleterPrimitive::Type>;

    static SharedPtr value;
};

MockDeleterPrimitive::SharedPtr MockDeleterPrimitive::value;

class Deleter {
public:
    void operator() (TestType* ptr) {
        MockDeleterPrimitive::value->free_TestType(ptr);
    }
};

class OpenSSLSharedPtrTest : public testing::Test {
protected:
    template <const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
    using ossl_shared_ptr__for_Testing = ossl_shared_ptr<TestType, P_DeleteStrategy, Deleter>;

    MockDeleterPrimitive::SharedPtr& mock = MockDeleterPrimitive::value;

    void SetUp() {
        mock = std::make_shared<MockDeleterPrimitive::Type>();
    }

    void TearDown() {
        mock = MockDeleterPrimitive::SharedPtr();
    }

    //static void SetUpTestCase() {
    //}

    //static void TearDownTestCase() {
    //}
};

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        (void)x;
        TestType& x_ref = *x;
        (void)x_ref;
        x->testMethod();
    }
}

} // namespace anonymous

} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

