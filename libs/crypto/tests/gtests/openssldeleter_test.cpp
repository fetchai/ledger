#include "crypto/openssl_memory.hpp"
#include "gtest/gtest.h"
//#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

namespace {

//class MockSSLFreeFunction {
// public:
//  MOCK_CONST_METHOD1(BIGNUM_free, void(BIGNUM*));
//  MOCK_CONST_METHOD1(BIGNUM_clear_free, void(BIGNUM*));
//  MOCK_CONST_METHOD1(BN_CTX_free, void(BN_CTX*));
//  MOCK_CONST_METHOD1(EC_POINT_free, void(EC_POINT*));
//  MOCK_CONST_METHOD1(EC_POINT_clear_free, void(EC_POINT*));
//  MOCK_CONST_METHOD1(EC_KEY_free, void(EC_KEY*));
//  MOCK_CONST_METHOD1(EC_GROUP_free, void(EC_GROUP*));
//  MOCK_CONST_METHOD1(EC_GROUP_clear_free, void(EC_GROUP*));
//};

//template <typename T, const eDeleteStrategy deleteStrategy = eDeleteStrategy::canonical>
//struct DeleterFunction {
//
//    static std::shared_ptr<MockSSLFreeFunctions> mock = std::shared_ptr<MockSSLFreeFunctions>();
//
//    static constexpr FunctionType<T, deleteStrategy> func() {
//       return [](T*) {mock->};
//    }
//    static constexpr FunctionType<T, deleteStrategy> functionPtr = default_fnc();
//};


class OpenSSLDeleterTest : public testing::Test {
protected:
    
    virtual void SetUp() {
    }
    
    virtual void TearDown() {
    }
};


TEST_F(OpenSSLDeleterTest, test_BIGNUM_free) {
    EXPECT_EQ(OpenSSLDeleter<BIGNUM>::DeleterFunction::functionPtr, &BN_free);
}

TEST_F(OpenSSLDeleterTest, test_BIGNUM_clear_free) {
    EXPECT_EQ((OpenSSLDeleter<BIGNUM, eDeleteStrategy::clearing>::DeleterFunction::functionPtr), &BN_clear_free);
}

TEST_F(OpenSSLDeleterTest, test_BN_CTX_free) {
    EXPECT_EQ(OpenSSLDeleter<BN_CTX>::DeleterFunction::functionPtr, &BN_CTX_free);
}

TEST_F(OpenSSLDeleterTest, test_EC_POINT_free) {
    EXPECT_EQ(OpenSSLDeleter<EC_POINT>::DeleterFunction::functionPtr, &EC_POINT_free);
}

TEST_F(OpenSSLDeleterTest, test_EC_POINT_clear_free) {
    EXPECT_EQ((OpenSSLDeleter<EC_POINT, eDeleteStrategy::clearing>::DeleterFunction::functionPtr), &EC_POINT_clear_free);
}

TEST_F(OpenSSLDeleterTest, test_EC_KEY_free) {
    EXPECT_EQ(OpenSSLDeleter<EC_KEY>::DeleterFunction::functionPtr, &EC_KEY_free);
}

TEST_F(OpenSSLDeleterTest, test_EC_GROUP_free) {
    EXPECT_EQ(OpenSSLDeleter<EC_GROUP>::DeleterFunction::functionPtr, &EC_GROUP_free);
}

TEST_F(OpenSSLDeleterTest, test_EC_GROUP_clear_free) {
    EXPECT_EQ((OpenSSLDeleter<EC_GROUP, eDeleteStrategy::clearing>::DeleterFunction::functionPtr), &EC_GROUP_clear_free);
}


} // namespace anonymous
} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

