#include "crypto/openssl_memory.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

namespace {

class DeleterTest : public testing::Test {
protected:
    
    virtual void SetUp() {
    }
    
    virtual void TearDown() {
    }
};


TEST_F(DeleterTest, test_BIGNUM_free) {
    EXPECT_EQ(Deleter<BIGNUM>::function, &BN_free);
}

TEST_F(DeleterTest, test_BIGNUM_clear_free) {
    EXPECT_EQ((Deleter<BIGNUM, eDeleteStrategy::clearing>::function), &BN_clear_free);
}

TEST_F(DeleterTest, test_BN_CTX_free) {
    EXPECT_EQ(Deleter<BN_CTX>::function, &BN_CTX_free);
}

TEST_F(DeleterTest, test_EC_POINT_free) {
    EXPECT_EQ(Deleter<EC_POINT>::function, &EC_POINT_free);
}

TEST_F(DeleterTest, test_EC_POINT_clear_free) {
    EXPECT_EQ((Deleter<EC_POINT, eDeleteStrategy::clearing>::function), &EC_POINT_clear_free);
}

TEST_F(DeleterTest, test_EC_KEY_free) {
    EXPECT_EQ(Deleter<EC_KEY>::function, &EC_KEY_free);
}

TEST_F(DeleterTest, test_EC_GROUP_free) {
    EXPECT_EQ(Deleter<EC_GROUP>::function, &EC_GROUP_free);
}

TEST_F(DeleterTest, test_EC_GROUP_clear_free) {
    EXPECT_EQ((Deleter<EC_GROUP, eDeleteStrategy::clearing>::function), &EC_GROUP_clear_free);
}




TEST_F(DeleterTest, test_BIGNUM_free_2) {
    EXPECT_EQ(Deleter2<>::getFreeFunction<BIGNUM>(), &BN_free);
}


} // namespace anonymous

} // namespace detail
} // namespace memory
} // namespace openssl
} // namespace crypto
} // namespace fetch

