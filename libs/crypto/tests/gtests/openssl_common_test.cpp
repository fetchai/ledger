#include "crypto/openssl_common.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

class ECDSACurveTest : public testing::Test {
protected:

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }


    template<int P_ECDSA_Curve_NID>
    void test_ECDSACurve(
        const char * const expected_sn,
        const std::size_t  expected_privateKeySize,
        const std::size_t  expected_publicKeySize,
        const std::size_t  expected_signatureSize) {
        using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>; 
        EXPECT_EQ(ecdsa_curve_type::nid, P_ECDSA_Curve_NID);
        EXPECT_EQ(expected_sn, ecdsa_curve_type::sn);
        EXPECT_EQ(expected_privateKeySize, ecdsa_curve_type::privateKeySize);
        EXPECT_EQ(expected_publicKeySize, ecdsa_curve_type::publicKeySize);
        EXPECT_EQ(expected_signatureSize, ecdsa_curve_type::signatureSize);
}

};


TEST_F(ECDSACurveTest, test_ECDSACurve_for_NID_secp256k1) {
    test_ECDSACurve<NID_secp256k1>(
        SN_secp256k1,
        32,
        64,
        64);
}

} // namespace anonymous

} // namespace openssl
} // namespace crypto
} // namespace fetch

