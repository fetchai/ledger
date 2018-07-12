#include "crypto/openssl_memory.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <functional>
#include <memory>
namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

namespace {

//using namespace ::testing;
using ::testing::StrictMock;

class MockDeleterPrimitive {
public:
    MOCK_CONST_METHOD1(BIGNUM_free, void(BIGNUM*));
    MOCK_CONST_METHOD1(BIGNUM_clear_free, void(BIGNUM*));
    MOCK_CONST_METHOD1(BN_CTX_free, void(BN_CTX*));
    MOCK_CONST_METHOD1(EC_POINT_free, void(EC_POINT*));
    MOCK_CONST_METHOD1(EC_POINT_clear_free, void(EC_POINT*));
    MOCK_CONST_METHOD1(EC_KEY_free, void(EC_KEY*));
    MOCK_CONST_METHOD1(EC_GROUP_free, void(EC_GROUP*));
    MOCK_CONST_METHOD1(EC_GROUP_clear_free, void(EC_GROUP*));

    using Type = StrictMock<MockDeleterPrimitive>;
    using SharedPtr = std::shared_ptr<MockDeleterPrimitive::Type>;

    static SharedPtr value;
};

MockDeleterPrimitive::SharedPtr MockDeleterPrimitive::value;

template <typename T
        , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
struct StaticMockDeleterPrimitive;

template<>
struct StaticMockDeleterPrimitive<BIGNUM> {
    static void functio(BIGNUM* ptr) {
        MockDeleterPrimitive::value->BIGNUM_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<BIGNUM, eDeleteStrategy::clearing> {
    static void function(BIGNUM* ptr) {
        MockDeleterPrimitive::value->BIGNUM_clear_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<BN_CTX> {
    static void function(BN_CTX* ptr) {
        MockDeleterPrimitive::value->BN_CTX_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<EC_POINT> {
    static void function(EC_POINT* ptr) {
        MockDeleterPrimitive::value->EC_POINT_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<EC_POINT, eDeleteStrategy::clearing> {
    static void function(EC_POINT* ptr) {
        MockDeleterPrimitive::value->EC_POINT_clear_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<EC_KEY> {
    static void function(EC_KEY* ptr) {
        MockDeleterPrimitive::value->EC_KEY_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<EC_GROUP> {
    static void function(EC_GROUP* ptr) {
        MockDeleterPrimitive::value->EC_GROUP_free(ptr);
    }
};

template<>
struct StaticMockDeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing> {
    static void function(EC_GROUP* ptr) {
        MockDeleterPrimitive::value->EC_GROUP_clear_free(ptr);
    }
};

template <typename T
         , const eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical>
using OpenSSLDeleter_forTesting = OpenSSLDeleter<T
                                              , P_DeleteStrategy
                                              , StaticMockDeleterPrimitive<T, P_DeleteStrategy>>;


class OpenSSLDeleterTest : public testing::Test {
protected:

    MockDeleterPrimitive::SharedPtr& mock = MockDeleterPrimitive::value;

    void SetUp() {
        mock = std::make_shared<MockDeleterPrimitive::Type>();
    }

    void TearDown() {
    }

    //static void SetUpTestCase() {
    //}

    //static void TearDownTestCase() {
    //}
};






//TEST_F(OpenSSLDeleterTest, test_BIGNUM_free) {
//    
//    OpenSSLDeleter_forTesting
//
//    EXPECT_EQ(OpenSSLDeleter<BIGNUM>::DeleterFunction::functionPtr, &BN_free);
//}

//TEST_F(OpenSSLDeleterTest, test_BIGNUM_clear_free) {
//    EXPECT_EQ((OpenSSLDeleter<BIGNUM, eDeleteStrategy::clearing>::DeleterFunction::functionPtr), &BN_clear_free);
//}
//
//TEST_F(OpenSSLDeleterTest, test_BN_CTX_free) {
//    EXPECT_EQ(OpenSSLDeleter<BN_CTX>::DeleterFunction::functionPtr, &BN_CTX_free);
//}
//
//TEST_F(OpenSSLDeleterTest, test_EC_POINT_free) {
//    EXPECT_EQ(OpenSSLDeleter<EC_POINT>::DeleterFunction::functionPtr, &EC_POINT_free);
//}
//
//TEST_F(OpenSSLDeleterTest, test_EC_POINT_clear_free) {
//    EXPECT_EQ((OpenSSLDeleter<EC_POINT, eDeleteStrategy::clearing>::DeleterFunction::functionPtr), &EC_POINT_clear_free);
//}
//
//TEST_F(OpenSSLDeleterTest, test_EC_KEY_free) {
//    EXPECT_EQ(OpenSSLDeleter<EC_KEY>::DeleterFunction::functionPtr, &EC_KEY_free);
//}
//
//TEST_F(OpenSSLDeleterTest, test_EC_GROUP_free) {
//    EXPECT_EQ(OpenSSLDeleter<EC_GROUP>::DeleterFunction::functionPtr, &EC_GROUP_free);
//}
//
//TEST_F(OpenSSLDeleterTest, test_EC_GROUP_clear_free) {
//    EXPECT_EQ((OpenSSLDeleter<EC_GROUP, eDeleteStrategy::clearing>::DeleterFunction::functionPtr), &EC_GROUP_clear_free);
//}


} // namespace anonymous

} // namespace detail
} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

