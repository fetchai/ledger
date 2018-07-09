#include "crypto/openssl_memory.hpp"
//#include "crypto/ecdsa.hpp"
//#include "core/byte_array/encoders.hpp"

#include "gtest/gtest.h"

//#include <iostream>


namespace fetch {
namespace crypto {
namespace openssl {

namespace {
// To use a test fixture, derive a class from testing::Test.
class OpenSSLMemoryTest : public testing::Test {
protected:
    
    virtual void SetUp() {
    }
    
    virtual void TearDown() {
    }
};

// Tests the default c'tor.
TEST_F(OpenSSLMemoryTest, test1) {
    auto constexpr x = 0u;
    EXPECT_EQ(0u,x);
}

} // namespace anonymous
} // namespace openssl
} // namespace crypto
} // namespace fetch

