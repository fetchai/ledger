#include "crypto/openssl_memory.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

struct TestType {};

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

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_construction) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        (void)x;
    }
}

TEST_F(OpenSSLSharedPtrTest, test_Deleter_not_called_for_empty_smart_ptr) {
    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x;
        (void)x;
    }
}

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_reset) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        x.reset();
    }
}

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_reset_with_specific_pointer) {
    TestType testValue;
    TestType testValue2;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
    EXPECT_CALL(*mock, free_TestType(&testValue2)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        x.reset(&testValue2);
    }
}

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_swap) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        ossl_shared_ptr__for_Testing<> y;
        x.swap(y);
    }
}

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_assign) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        ossl_shared_ptr__for_Testing<> y;
        x = y;
    }
}

TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_copy_construct) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);
        ossl_shared_ptr__for_Testing<> y(x);
    }
}

} // namespace anonymous

} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

