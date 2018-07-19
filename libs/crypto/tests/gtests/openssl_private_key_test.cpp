#include "crypto/openssl_ecdsa_private_key.hpp"

#include "openssl/ec.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

class ECDCSAPrivateKeyTest : public testing::Test {
protected:
    void SetUp() {
    }

    void TearDown() {
    }
};


//TODO: This test is rubbish, we need hardoced test key data pair
//      in binary format - private ikey and corresponding public key
//      in order to be able to compare calculated/acquired public key
//      agains hardcoded expected public key.
TEST_F(ECDCSAPrivateKeyTest, test_instantiation_of_private_key) {
    const fetch::byte_array::ConstByteArray priv_key_data = {
        0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73, 0x16, 0x73, 0x62,
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7,
        0x27, 0xf3, 0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42};
    const std::string priv_key_hex_str = "16260783e40b16731673622ac8a5b045fc3ea4af70f727f3f9e92bdd3a1ddc42";

    //* Production code:
    ECDSAPrivateKey<> x(priv_key_data);
    ECDSAPrivateKey<> y(priv_key_hex_str);
    //std::cerr << y.publicKey().keyAsBin() << std::endl;
    //std::cerr << x.publicKey().keyAsBin() << std::endl;

    //* Expectations:
    EXPECT_TRUE(x.key());
    EXPECT_TRUE(y.key());

    EXPECT_EQ(x.publicKey().keyAsBin(), y.publicKey().keyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_generation_of_private_key) {
    //* Production code:
    ECDSAPrivateKey<> x;
    ECDSAPrivateKey<> y;

    //* Expectations:
    EXPECT_TRUE(x.key());
    EXPECT_TRUE(x.publicKey().keyAsEC_POINT());

    EXPECT_TRUE(y.key());
    EXPECT_TRUE(y.publicKey().keyAsEC_POINT());

    EXPECT_NE(x.publicKey().keyAsBin(), y.publicKey().keyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_key_conversion_to_byte_array) {
    const fetch::byte_array::ConstByteArray priv_key_data = {
        0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73, 0x16, 0x73, 0x62,
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7,
        0x27, 0xf3, 0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42};

    //* Production code:
    ECDSAPrivateKey<> x(priv_key_data);
    //std::cerr << y.publicKey().keyAsBin() << std::endl;

    //* Expectations:
    EXPECT_TRUE(x.key());

    EXPECT_EQ(priv_key_data, x.KeyAsBin());
}

//TODO: Add more tests

} // namespace anonymous

} // namespace openssl
} // namespace crypto
} // namespace fetch

