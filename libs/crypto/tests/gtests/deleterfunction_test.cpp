#include "crypto/openssl_memory.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

namespace {

class DeleterFunctionTest : public testing::Test {
protected:
    
    virtual void SetUp() {
    }
    
    virtual void TearDown() {
    }
};


TEST_F(DeleterFunctionTest, test_BIGNUM_free) {
    EXPECT_EQ(DeleterFunction<BIGNUM>::functionPtr, &BN_free);
}

TEST_F(DeleterFunctionTest, test_BIGNUM_clear_free) {
    EXPECT_EQ((DeleterFunction<BIGNUM, eDeleteStrategy::clearing>::functionPtr), &BN_clear_free);
}

TEST_F(DeleterFunctionTest, test_BN_CTX_free) {
    EXPECT_EQ(DeleterFunction<BN_CTX>::functionPtr, &BN_CTX_free);
}

TEST_F(DeleterFunctionTest, test_EC_POINT_free) {
    EXPECT_EQ(DeleterFunction<EC_POINT>::functionPtr, &EC_POINT_free);
}

TEST_F(DeleterFunctionTest, test_EC_POINT_clear_free) {
    EXPECT_EQ((DeleterFunction<EC_POINT, eDeleteStrategy::clearing>::functionPtr), &EC_POINT_clear_free);
}

TEST_F(DeleterFunctionTest, test_EC_KEY_free) {
    EXPECT_EQ(DeleterFunction<EC_KEY>::functionPtr, &EC_KEY_free);
}

TEST_F(DeleterFunctionTest, test_EC_GROUP_free) {
    EXPECT_EQ(DeleterFunction<EC_GROUP>::functionPtr, &EC_GROUP_free);
}

TEST_F(DeleterFunctionTest, test_EC_GROUP_clear_free) {
    EXPECT_EQ((DeleterFunction<EC_GROUP, eDeleteStrategy::clearing>::functionPtr), &EC_GROUP_clear_free);
}

} // namespace anonymous

} // namespace detail
} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

