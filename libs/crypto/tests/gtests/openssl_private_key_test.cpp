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

TEST_F(ECDCSAPrivateKeyTest, test_creation_of_private_key) {
    fetch::byte_array::ConstByteArray priv_key_data = {
        0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73, 0x16, 0x73, 0x62,
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7,
        0x27, 0xf3, 0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42};
    std::string priv_key_hex_str = "16260783e40b16731673622ac8a5b045fc3ea4af70f727f3f9e92bdd3a1ddc42";

    //* Production code
    ECDSAPrivateKey<> x(priv_key_data);
    EXPECT_TRUE(x.key());

    ECDSAPrivateKey<> y(priv_key_hex_str);
    EXPECT_TRUE(y.key());

    std::cerr << "x.key().get() = " << x.key().get() << std::endl;
    std::cerr << "x.key() len = " << i2d_ECPrivateKey(x.key().get(), NULL) << std::endl;
    std::vector<unsigned char> x_out((std::size_t)i2d_ECPrivateKey(x.key().get(), NULL), 0);
    std::cerr << "x_out = ";
    for (auto itm : x_out) {
        std::cerr << itm << ", ";
    }
    std::cerr << std::endl;

    unsigned char *_x_out = &x_out.front();
    if(!i2d_ECPrivateKey(x.key().get(), &_x_out)) {
    //    throw std::runtime_error("Error: i2d_ECPrivateKey(x, ..) fails.");
    }
    std::cerr << "x_out(after) = ";
    for (auto itm : x_out) {
        std::cerr << itm << ", ";
    }
    std::cerr << std::endl;

    std::vector<unsigned char> y_out((std::size_t)i2d_ECPrivateKey(y.key().get(), NULL), 0);
    unsigned char *_y_out = &y_out.front();
    if(!i2d_ECPrivateKey(y.key().get(), &_y_out)) {
    //    throw std::runtime_error("Error: i2d_ECPrivateKey(y, ..) fails.");
    }

    EXPECT_EQ(x_out, y_out);
}

//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_called_after_construction) {
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
//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_not_called_for_empty_smart_ptr) {
//    {
//        //* Production code
//        ossl_shared_ptr__for_Testing<> x;
//        (void)x;
//    }
//}
//
//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_called_after_reset) {
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
//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_called_after_reset_with_specific_pointer) {
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
//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_called_after_swap) {
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
//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_called_after_assign) {
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
//TEST_F(ECDCSAPrivateKeyTest, test_Deleter_called_after_copy_construct) {
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

} // namespace openssl
} // namespace crypto
} // namespace fetch

