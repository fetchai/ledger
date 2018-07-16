#include "crypto/openssl_context_detail.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace context {
namespace detail {

namespace {

class SessionPrimitiveTest : public ::testing::Test {
protected:

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};


TEST_F(SessionPrimitiveTest, test_BN_CTX_start) {
    EXPECT_EQ(SessionPrimitive<BN_CTX>::start, &BN_CTX_start);
}

TEST_F(SessionPrimitiveTest, test_BN_CTX_end) {
    EXPECT_EQ(SessionPrimitive<BN_CTX>::end, &BN_CTX_end);
}

} // namespace anonymous

} // namespace detail
} // namespace context
} // namespace openssl
} // namespace crypto
} // namespace fetch

