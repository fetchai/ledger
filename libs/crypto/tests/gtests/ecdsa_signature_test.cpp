#include "crypto/ecdsa_signature.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

class ECDCSASignatureTest : public testing::Test {
protected:
    const fetch::byte_array::ConstByteArray priv_key_data = {
        0x92, 0xad, 0x61, 0xcf, 0xfc, 0xb9, 0x2a, 0x17,
        0x02, 0xa3, 0xd6, 0x03, 0xa0, 0x0d, 0x6e, 0xb3,
        0xad, 0x92, 0x0f, 0x8c, 0xec, 0x43, 0xda, 0x41,
        0x8f, 0x01, 0x04, 0xc6, 0xc6, 0xc9, 0xe0, 0x5e};

    const fetch::byte_array::ConstByteArray test_data = {
        0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4,
        0xaf, 0x70, 0xf7, 0x34, 0xaa, 0xda, 0x83, 0xe5,
        0x0b, 0x16, 0xff, 0x16, 0x73, 0x62, 0x27, 0xf3,
        0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42,
        0x01, 0xaa, 0x05};

    void SetUp() {
    }

    void TearDown() {
    }


    template<eECDSABinaryDataFormat P_ECDSASignatureBinaryDataFormat>
    void test_wrong_signature_fails_to_verify() {
        //* Production code:
        openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

        using ecdsa_signature_type = ECDSASignature<P_ECDSASignatureBinaryDataFormat>;
        ecdsa_signature_type signature {ecdsa_signature_type::Sign(priv_key, test_data)};

        byte_array::ByteArray signature_copy {signature.signature()};

        ASSERT_TRUE(signature_copy.size() > 0);

        //* Modify the correct signature to invalidate it
        signature_copy[signature_copy.size()-1] += 1;
        ecdsa_signature_type wrong_signature {signature_copy};

        const auto verification_result =
            wrong_signature.Verify(priv_key.publicKey(), test_data);

        //* Expectations:
        EXPECT_FALSE(verification_result);
    }

    template<eECDSABinaryDataFormat P_ECDSASignatureBinaryDataFormat>
    void test_construct_signature_from_binary_data() {
        //* Production code:
        openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

        using ecdsa_signature_type = ECDSASignature<P_ECDSASignatureBinaryDataFormat>;
        ecdsa_signature_type signature {ecdsa_signature_type::Sign(priv_key, test_data)};

        //* Verify that acquired signature is correct:
        ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data));

        //* Re-construct signature from binary from:
        ecdsa_signature_type signature_from_canonical_bin {signature.signature()};

        //* Verify that re-constructed signature is able to verify:
        EXPECT_TRUE(signature_from_canonical_bin.Verify(priv_key.publicKey(), test_data));
    }
};


TEST_F(ECDCSASignatureTest, test_sign_verify_cycle) {
    //* Production code:
    openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

    using ecdsa_signature_type = ECDSASignature<>;
    ecdsa_signature_type signature {ecdsa_signature_type::Sign(priv_key, test_data)};

    const auto verification_result =
        signature.Verify(priv_key.publicKey(), test_data);

    //* Expectations:
    EXPECT_TRUE(verification_result);
}

TEST_F(ECDCSASignatureTest, test_wrong_signature_fails_to_verify__canonical) {
    test_wrong_signature_fails_to_verify<eECDSABinaryDataFormat::canonical>();
}

TEST_F(ECDCSASignatureTest, test_wrong_signature_fails_to_verify__DER) {
    test_wrong_signature_fails_to_verify<eECDSABinaryDataFormat::DER>();
}

TEST_F(ECDCSASignatureTest, test_invalid_DER_signature_causes_exception) {
    //* Production code:
    openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

    using ecdsa_signature_type = ECDSASignature<eECDSABinaryDataFormat::DER>;
    ecdsa_signature_type signature {ecdsa_signature_type::Sign(priv_key, test_data)};

    //* Verify that acquired signature is correct:
    ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data));


    //* Invalidating signature by modifying it's first byte of DER format
    byte_array::ByteArray signature_copy {signature.signature()};

    ASSERT_TRUE(signature_copy.size() > 0);

    //* Modify the correct signature to invalidate it
    signature_copy[0] += 1;

    EXPECT_THROW (
        {
            ecdsa_signature_type wrong_signature {signature_copy};
        },
        std::runtime_error
    );
}

TEST_F(ECDCSASignatureTest, test_wrong_data_fails_to_verify) {
    //* Production code:
    openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

    using ecdsa_signature_type = ECDSASignature<>;
    ecdsa_signature_type signature {ecdsa_signature_type::Sign(priv_key, test_data)};

    //* Verify that acquired signature is correct:
    ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data));

    byte_array::ByteArray modified_data = test_data.Copy();
    ASSERT_TRUE(modified_data.size() > 0);

    //* Modify original data to make verification fail
    modified_data[0] += 1;

    const auto verification_result =
        signature.Verify(priv_key.publicKey(), modified_data);

    //* Expectations:
    EXPECT_FALSE(verification_result);
}

TEST_F(ECDCSASignatureTest, test_construct_signature_from_binary_data__canonical) {
    test_construct_signature_from_binary_data<eECDSABinaryDataFormat::canonical>();
}

TEST_F(ECDCSASignatureTest, test_construct_signature_from_binary_data__DER) {
    test_construct_signature_from_binary_data<eECDSABinaryDataFormat::DER>();
}

TEST_F(ECDCSASignatureTest, test_canonical_signature_binary_representation_has_expected_length) {
    //* Production code:
    openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

    using ecdsa_signature_type = ECDSASignature<eECDSABinaryDataFormat::canonical>;
    ecdsa_signature_type signature {ecdsa_signature_type::Sign(priv_key, test_data)};

    //* Verify that acquired signature is correct:
    ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data));

    //* Create signature from Canonical binary from:
    ecdsa_signature_type signature_from_canonical_bin {signature.signature()};

    //* Verify that signature reconstructed from canonical bin agle to verify:
    ASSERT_TRUE(signature_from_canonical_bin.Verify(priv_key.publicKey(), test_data));

    //* Expectations:
    EXPECT_EQ(ecdsa_signature_type::ecdsa_curve_type::signatureSize, signature.signature().size());
}

} // namespace anonymous

} // namespace openssl
} // namespace crypto
} // namespace fetch

