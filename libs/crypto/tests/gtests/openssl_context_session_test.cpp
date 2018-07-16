#include "crypto/openssl_context_session.hpp"

#include "gtest/gtest.h"
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
    void testMethod() {
    }
};

class MockContextPrimitive {
public:
    MOCK_CONST_METHOD1(start, void(TestType*));
    MOCK_CONST_METHOD1(end, void(TestType*));

    using Type = StrictMock<MockContextPrimitive>;
    using SharedPtr = std::shared_ptr<MockContextPrimitive::Type>;

    static SharedPtr value;
};

MockContextPrimitive::SharedPtr MockContextPrimitive::value;


template <typename T>
struct StaticMockContextPrimitive;

template<>
struct StaticMockContextPrimitive<TestType> {
    static void start(TestType* ptr) {
        MockContextPrimitive::value->start(ptr);
    }
    static void end(TestType* ptr) {
        MockContextPrimitive::value->end(ptr);
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
    template <const memory::eDeleteStrategy P_DeleteStrategy = memory::eDeleteStrategy::canonical>
    using ossl_shared_ptr__for_Testing = memory::ossl_shared_ptr<TestType, P_DeleteStrategy, Deleter>;

    MockDeleterPrimitive::SharedPtr& deleterMock = MockDeleterPrimitive::value;
    MockContextPrimitive::SharedPtr& contextMock = MockContextPrimitive::value;

    void SetUp() {
        deleterMock = std::make_shared<MockDeleterPrimitive::Type>();
        contextMock = std::make_shared<MockContextPrimitive::Type>();
    }

    void TearDown() {
        deleterMock = MockDeleterPrimitive::SharedPtr();
        contextMock = MockContextPrimitive::SharedPtr();
    }

    //static void SetUpTestCase() {
    //}

    //static void TearDownTestCase() {
    //}
};

//TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_construction) {
//    TestType testValue;
//
//    //* Expctation
//    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
//
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x(&testValue);
//        (void)x;
//    }
//}
//
//TEST_F(OpenSSLSharedPtrTest, test_Deleter_not_called_for_empty_smart_ptr) {
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x;
//        (void)x;
//    }
//}
//
//TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_reset) {
//    TestType testValue;
//
//    //* Expctation
//    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
//
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x(&testValue);
//        x.reset();
//    }
//}
//
//TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_reset_with_specific_pointer) {
//    TestType testValue;
//    TestType testValue2;
//
//    //* Expctation
//    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
//    EXPECT_CALL(*mock, free_TestType(&testValue2)).WillOnce(Return());
//
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x(&testValue);
//        x.reset(&testValue2);
//    }
//}
//
//TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_swap) {
//    TestType testValue;
//
//    //* Expctation
//    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
//
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x(&testValue);
//        ossl_shared_ptr__for_Testing<> y;
//        x.swap(y);
//    }
//}
//
//TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_assign) {
//    TestType testValue;
//
//    //* Expctation
//    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
//
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x(&testValue);
//        ossl_shared_ptr__for_Testing<> y;
//        x = y;
//    }
//}
//
//TEST_F(OpenSSLSharedPtrTest, test_Deleter_called_after_copy_construct) {
//    TestType testValue;
//
//    //* Expctation
//    EXPECT_CALL(*mock, free_TestType(&testValue)).WillOnce(Return());
//
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x(&testValue);
//        ossl_shared_ptr__for_Testing<> y(x);
//    }
//}

} // namespace anonymous

} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

