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



//class MockDeleterPrimitive {
//public:
//    MOCK_CONST_METHOD1(free_TestType, void(TestType*));
//
//    using Type = StrictMock<MockDeleterPrimitive>;
//    using SharedPtr = std::shared_ptr<MockDeleterPrimitive::Type>;
//
//    static SharedPtr value;
//};
//
//MockDeleterPrimitive::SharedPtr MockDeleterPrimitive::value;
//
//class Deleter {
//public:
//    void operator() (TestType* ptr) {
//        MockDeleterPrimitive::value->free_TestType(ptr);
//    }
//};

class Deleter {
public:
    void operator() (TestType* ptr) {
    }
};



class OpenSSLContextSessionTest : public testing::Test {
protected:
    template <const memory::eDeleteStrategy P_DeleteStrategy = memory::eDeleteStrategy::canonical>
    using ossl_shared_ptr__for_Testing = memory::ossl_shared_ptr<TestType, P_DeleteStrategy, Deleter>;
    using Session__for_Testing = Session<TestType, StaticMockContextPrimitive<TestType>, ossl_shared_ptr__for_Testing<>>;

    //MockDeleterPrimitive::SharedPtr& deleterMock = MockDeleterPrimitive::value;
    MockContextPrimitive::SharedPtr& contextMock = MockContextPrimitive::value;

    void SetUp() {
        //deleterMock = std::make_shared<MockDeleterPrimitive::Type>();
        contextMock = std::make_shared<MockContextPrimitive::Type>();
    }

    void TearDown() {
        //deleterMock = MockDeleterPrimitive::SharedPtr();
        contextMock = MockContextPrimitive::SharedPtr();
    }

    //static void SetUpTestCase() {
    //}

    //static void TearDownTestCase() {
    //}
};



TEST_F(OpenSSLContextSessionTest, test_Session_basic_scenario_constructro_and_destructor) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*contextMock, start(&testValue)).WillOnce(Return());
    EXPECT_CALL(*contextMock, end(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);

        Session__for_Testing session(x);
    }
}

TEST_F(OpenSSLContextSessionTest, test_Session_constructor_and_end) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*contextMock, start(&testValue)).WillOnce(Return());
    EXPECT_CALL(*contextMock, end(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);

        Session__for_Testing session(x);
        session.end();
    }
}

TEST_F(OpenSSLContextSessionTest, test_Session_started_and_destructor) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*contextMock, end(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);

        Session__for_Testing session(x, true);
    }
}

TEST_F(OpenSSLContextSessionTest, test_Session_started_and_end) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*contextMock, end(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);

        Session__for_Testing session(x, true);
        session.end();
    }
}

TEST_F(OpenSSLContextSessionTest, test_Session_constructor_and_start_and_destructor) {
    TestType testValue;

    //* Expctation
    EXPECT_CALL(*contextMock, start(&testValue)).WillOnce(Return());
    EXPECT_CALL(*contextMock, end(&testValue)).WillOnce(Return());

    {
        //* Production code
        ossl_shared_ptr__for_Testing<> x(&testValue);

        Session__for_Testing session(x);
        session.start();
    }
}

} // namespace anonymous

} // namespace context
} // namespace openssl
} // namespace crypto
} // namespace fetch

