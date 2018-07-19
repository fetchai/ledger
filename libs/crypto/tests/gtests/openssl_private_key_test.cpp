#include "crypto/openssl_ecdsa_private_key.hpp"

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
    //std::cerr << y.publicKey().KeyAsBin() << std::endl;
    //std::cerr << x.publicKey().KeyAsBin() << std::endl;

    //* Expectations:
    EXPECT_TRUE(x.key());
    EXPECT_TRUE(y.key());

    EXPECT_EQ(x.publicKey().KeyAsBin(), y.publicKey().KeyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_instantiation_of_private_key_2) {
    const fetch::byte_array::ConstByteArray priv_key_data = {0x92, 0xad, 0x61, 0xcf, 0xfc, 0xb9, 0x2a, 0x17, 0x2, 0xa3, 0xd6, 0x3, 0xa0, 0xd, 0x6e, 0xb3, 0xad, 0x92, 0xf, 0x8c, 0xec, 0x43, 0xda, 0x41, 0x8f, 0x1, 0x4, 0xc6, 0xc6, 0xc9, 0xe0, 0x5e};

    const fetch::byte_array::ConstByteArray public_key_data = {0x4, 0x55, 0x5a, 0x38, 0xa4, 0x2d, 0xb2, 0x9d, 0x5, 0xcd, 0xe3, 0xea, 0xa0, 0x93, 0x7, 0x89, 0x46, 0x16, 0xb5, 0xa2, 0xb5, 0xa3, 0x2, 0xe9, 0x13, 0xee, 0xa2, 0x6d, 0x3, 0x48, 0xec, 0x5b, 0x5c, 0x7, 0x30, 0x2d, 0xfc, 0xdb, 0xd5, 0xcd, 0xa1, 0x73, 0x74, 0xcd, 0x2f, 0x6b, 0xec, 0xcf, 0xc4, 0x67, 0xa1, 0x51, 0x3a, 0xa1, 0xf7, 0xb4, 0xeb, 0x3f, 0x1c, 0x0, 0x6b, 0x6e, 0xb6, 0x2b, 0x51};

    //* Production code:
    ECDSAPrivateKey<> x(priv_key_data);

    //* Expectations:
    EXPECT_TRUE(x.key());
    EXPECT_TRUE(x.publicKey().Key());

    EXPECT_EQ(priv_key_data, x.KeyAsBin());
    EXPECT_EQ(public_key_data, x.publicKey().KeyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_generation_of_private_key) {
    //* Production code:
    ECDSAPrivateKey<> x;
    ECDSAPrivateKey<> y;
    
    
    
    //std::cerr << "ECDCSAPrivateKeyTest::test_generation_of_private_key(...): encoded PRIVATE key = " << toHexStr(x.KeyAsBin()) << std::endl;
    //std::cerr << "ECDCSAPrivateKeyTest::test_generation_of_private_key(...): encoded PUBLIC key  = " << toHexStr(x.publicKey().KeyAsBin()) << std::endl;

    //* Expectations:
    EXPECT_TRUE(x.key());
    EXPECT_TRUE(x.publicKey().keyAsEC_POINT());

    EXPECT_TRUE(y.key());
    EXPECT_TRUE(y.publicKey().keyAsEC_POINT());

    EXPECT_NE(x.publicKey().KeyAsBin(), y.publicKey().KeyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_key_conversion_to_byte_array) {
    const fetch::byte_array::ConstByteArray priv_key_data = {
        0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73, 0x16, 0x73, 0x62,
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7,
        0x27, 0xf3, 0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42};

    //* Production code:
    ECDSAPrivateKey<> x(priv_key_data);
    //std::cerr << y.publicKey().KeyAsBin() << std::endl;

    //* Expectations:
    EXPECT_TRUE(x.key());

    EXPECT_EQ(priv_key_data, x.KeyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_public_key_conversions) {
    const fetch::byte_array::ConstByteArray priv_key_data = {
        0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73, 0x16, 0x73, 0x62,
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7,
        0x27, 0xf3, 0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42};

    //* Production code:
    ECDSAPrivateKey<> x(priv_key_data);
    //std::cerr << y.publicKey().KeyAsBin() << std::endl;

    
    
    EXPECT_EQ(priv_key_data, x.KeyAsBin());
}

//TODO: Add more tests

} // namespace anonymous

} // namespace openssl
} // namespace crypto
} // namespace fetch

