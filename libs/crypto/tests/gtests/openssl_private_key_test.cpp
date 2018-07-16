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

TEST_F(ECDCSAPrivateKeyTest, test_creation_of_private_key) {
    fetch::byte_array::ConstByteArray priv_key_data = {
        0x16, 0x26, 0x07, 0x83, 0xe4, 0x0b, 0x16, 0x73, 0x16, 0x73, 0x62,
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7,
        0x27, 0xf3, 0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42};

    //* Production code
    ECDSAPrivateKey<> x(priv_key_data);
    EXPECT_TRUE(x.get());
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

