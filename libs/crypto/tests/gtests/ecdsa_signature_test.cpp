//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "crypto/ecdsa_signature.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

class ECDCSASignatureTest : public testing::Test
{
protected:
  const fetch::byte_array::ConstByteArray priv_key_data_ = {
      0x92, 0xad, 0x61, 0xcf, 0xfc, 0xb9, 0x2a, 0x17, 0x02, 0xa3, 0xd6,
      0x03, 0xa0, 0x0d, 0x6e, 0xb3, 0xad, 0x92, 0x0f, 0x8c, 0xec, 0x43,
      0xda, 0x41, 0x8f, 0x01, 0x04, 0xc6, 0xc6, 0xc9, 0xe0, 0x5e};

  const fetch::byte_array::ConstByteArray test_data_ = {
      0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7, 0x34,
      0xaa, 0xda, 0x83, 0xe5, 0x0b, 0x16, 0xff, 0x16, 0x73, 0x62, 0x27, 0xf3,
      0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42, 0x01, 0xaa, 0x05};

  void SetUp()
  {}

  void TearDown()
  {}

  template <eECDSAEncoding SIG_ENCODING, eECDSAEncoding KEY_ENCODING>
  void test_sign_verify_hash_cycle()
  {
    //* Production code:
    openssl::ECDSAPrivateKey<KEY_ENCODING> priv_key{
        openssl::ECDSAPrivateKey<eECDSAEncoding::bin>(priv_key_data_)};

    using ecdsa_signature_type          = ECDSASignature<SIG_ENCODING>;
    auto const &         test_hash_data = test_data_;
    ecdsa_signature_type signature{ecdsa_signature_type::SignHash(priv_key, test_hash_data)};

    const auto verification_result = signature.VerifyHash(priv_key.publicKey(), test_hash_data);

    //* Expectations:
    EXPECT_TRUE(verification_result);
  }

  template <eECDSAEncoding SIG_ENCODING, eECDSAEncoding KEY_ENCODING>
  void test_sign_verify_cycle()
  {
    //* Production code:
    openssl::ECDSAPrivateKey<KEY_ENCODING> priv_key{
        openssl::ECDSAPrivateKey<eECDSAEncoding::bin>(priv_key_data_)};

    using ecdsa_signature_type = ECDSASignature<SIG_ENCODING>;
    ecdsa_signature_type signature{ecdsa_signature_type::Sign(priv_key, test_data_)};

    const auto verification_result = signature.Verify(priv_key.publicKey(), test_data_);

    //* Expectations:
    EXPECT_TRUE(verification_result);
  }

  template <eECDSAEncoding ENCODING>
  void test_wrong_signature_fails_to_verify()
  {
    //* Production code:
    openssl::ECDSAPrivateKey<> priv_key(priv_key_data_);

    using ecdsa_signature_type = ECDSASignature<ENCODING>;
    ecdsa_signature_type signature{ecdsa_signature_type::Sign(priv_key, test_data_)};

    byte_array::ByteArray inv_sig_enc{signature.signature()};

    ASSERT_TRUE(inv_sig_enc.size() > 0);

    //* Modify the correct signature to invalidate it
    ++inv_sig_enc[inv_sig_enc.size() - 1u];
    ecdsa_signature_type wrong_signature{inv_sig_enc};

    const auto verification_result = wrong_signature.Verify(priv_key.publicKey(), test_data_);

    //* Expectations:
    EXPECT_FALSE(verification_result);
  }

  template <eECDSAEncoding ENCODING>
  void test_construct_signature_from_binary_data()
  {
    //* Production code:
    openssl::ECDSAPrivateKey<> priv_key(priv_key_data_);

    using ecdsa_signature_type = ECDSASignature<ENCODING>;
    ecdsa_signature_type signature{ecdsa_signature_type::Sign(priv_key, test_data_)};

    //* Verify that acquired signature is correct:
    ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data_));

    //* Re-construct signature from binary from:
    ecdsa_signature_type signature_from_canonical_bin{signature.signature()};

    //* Verify that re-constructed signature is able to verify:
    EXPECT_TRUE(signature_from_canonical_bin.Verify(priv_key.publicKey(), test_data_));
  }

  template <eECDSAEncoding ENCODING>
  void test_invalidated_signature()
  {
    // static_assert(ENCODING == eECDSAEncoding::DER, "");

    openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

    //* Production code:
    using ecdsa_signature_type = ECDSASignature<ENCODING>;
    ecdsa_signature_type signature{ecdsa_signature_type::Sign(priv_key, test_data_)};

    //* Verify that acquired signature is correct:
    ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data_));

    //* Invalidating signature by modifying it's first byte of it's format
    byte_array::ByteArray inv_sig_enc{signature.signature().Copy()};

    ASSERT_TRUE(inv_sig_enc.size() > 0);

    //* Modify the correct signature to invalidate it
    ++inv_sig_enc[0];

    //* It is not possible to invalidate (format-wise) canonical or bin encoded
    // signature, since it
    // does NOT contain any structural/format information except just pure data
    // (r & s values). Thus
    // it is only possible to make such signature not to verify.
    if (ENCODING == eECDSAEncoding::DER)
    {
      EXPECT_THROW({ ecdsa_signature_type inv_sig{std::move(inv_sig_enc)}; }, std::runtime_error);
    }
    else
    {
      //* This shall pass without exception
      ecdsa_signature_type inv_sig{std::move(inv_sig_enc)};
    }
  }

  template <eECDSAEncoding ENCODING>
  void test_wrong_data_fails_to_verify()
  {
    //* Production code:
    openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

    using ecdsa_signature_type = ECDSASignature<ENCODING>;
    ecdsa_signature_type signature{ecdsa_signature_type::Sign(priv_key, test_data_)};

    //* Verify that acquired signature is correct:
    ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data_));

    byte_array::ByteArray modified_data = test_data_.Copy();
    ASSERT_TRUE(modified_data.size() > 0);

    //* Modify original data to make verification fail
    ++modified_data[0];

    const auto verification_result = signature.Verify(priv_key.publicKey(), modified_data);

    //* Expectations:
    EXPECT_FALSE(verification_result);
  }
};

TEST_F(ECDCSASignatureTest, test_sign_verify_hash_cycle)
{
  test_sign_verify_hash_cycle<eECDSAEncoding::canonical, eECDSAEncoding::canonical>();
  test_sign_verify_hash_cycle<eECDSAEncoding::canonical, eECDSAEncoding::bin>();
  test_sign_verify_hash_cycle<eECDSAEncoding::canonical, eECDSAEncoding::DER>();

  test_sign_verify_hash_cycle<eECDSAEncoding::bin, eECDSAEncoding::canonical>();
  test_sign_verify_hash_cycle<eECDSAEncoding::bin, eECDSAEncoding::bin>();
  test_sign_verify_hash_cycle<eECDSAEncoding::bin, eECDSAEncoding::DER>();

  test_sign_verify_hash_cycle<eECDSAEncoding::DER, eECDSAEncoding::canonical>();
  test_sign_verify_hash_cycle<eECDSAEncoding::DER, eECDSAEncoding::bin>();
  test_sign_verify_hash_cycle<eECDSAEncoding::DER, eECDSAEncoding::DER>();
}

TEST_F(ECDCSASignatureTest, test_sign_verify_cycle)
{
  test_sign_verify_cycle<eECDSAEncoding::canonical, eECDSAEncoding::canonical>();
  test_sign_verify_cycle<eECDSAEncoding::canonical, eECDSAEncoding::bin>();
  test_sign_verify_cycle<eECDSAEncoding::canonical, eECDSAEncoding::DER>();

  test_sign_verify_cycle<eECDSAEncoding::bin, eECDSAEncoding::canonical>();
  test_sign_verify_cycle<eECDSAEncoding::bin, eECDSAEncoding::bin>();
  test_sign_verify_cycle<eECDSAEncoding::bin, eECDSAEncoding::DER>();

  test_sign_verify_cycle<eECDSAEncoding::DER, eECDSAEncoding::canonical>();
  test_sign_verify_cycle<eECDSAEncoding::DER, eECDSAEncoding::bin>();
  test_sign_verify_cycle<eECDSAEncoding::DER, eECDSAEncoding::DER>();
}

TEST_F(ECDCSASignatureTest, test_wrong_signature_fails_to_verify__canonical)
{
  test_wrong_signature_fails_to_verify<eECDSAEncoding::canonical>();
}

TEST_F(ECDCSASignatureTest, test_wrong_signature_fails_to_verify__DER)
{
  test_wrong_signature_fails_to_verify<eECDSAEncoding::DER>();
}

TEST_F(ECDCSASignatureTest, test_construct_signature_from_binary_data__canonical)
{
  test_construct_signature_from_binary_data<eECDSAEncoding::canonical>();
}

TEST_F(ECDCSASignatureTest, test_construct_signature_from_binary_data__DER)
{
  test_construct_signature_from_binary_data<eECDSAEncoding::DER>();
}

TEST_F(ECDCSASignatureTest, test_invalid_signature_causes_exception__DER)
{
  test_invalidated_signature<eECDSAEncoding::DER>();
}

TEST_F(ECDCSASignatureTest, test_bin_signature_does_not_invalidate)
{
  test_invalidated_signature<eECDSAEncoding::bin>();
}

TEST_F(ECDCSASignatureTest, test_canonical_signature_does_not_invalidate)
{
  test_invalidated_signature<eECDSAEncoding::canonical>();
}

TEST_F(ECDCSASignatureTest, test_wrong_data_fails_to_verify__canonical_sig)
{
  test_wrong_data_fails_to_verify<eECDSAEncoding::canonical>();
}

TEST_F(ECDCSASignatureTest, test_wrong_data_fails_to_verify__bin_sig)
{
  test_wrong_data_fails_to_verify<eECDSAEncoding::bin>();
}

TEST_F(ECDCSASignatureTest, test_wrong_data_fails_to_verify__DER_sig)
{
  test_wrong_data_fails_to_verify<eECDSAEncoding::DER>();
}

TEST_F(ECDCSASignatureTest, test_canonical_signature_binary_representation_has_expected_length)
{
  //* Production code:
  openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

  using ecdsa_signature_type = ECDSASignature<eECDSAEncoding::canonical>;
  ecdsa_signature_type signature{ecdsa_signature_type::Sign(priv_key, test_data_)};

  //* Verify that acquired signature is correct:
  ASSERT_TRUE(signature.Verify(priv_key.publicKey(), test_data_));

  //* Create signature from Canonical binary from:
  ecdsa_signature_type signature_from_canonical_bin{signature.signature()};

  //* Verify that signature reconstructed from canonical binary data is able to
  // verify:
  ASSERT_TRUE(signature_from_canonical_bin.Verify(priv_key.publicKey(), test_data_));

  //* Expectations:
  EXPECT_EQ(ecdsa_signature_type::ecdsa_curve_type::signatureSize, signature.signature().size());
}

TEST_F(ECDCSASignatureTest, test_moving_semantics_constructor)
{
  openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

  ECDSASignature<eECDSAEncoding::canonical> sig_0{
      ECDSASignature<eECDSAEncoding::canonical>::Sign(priv_key, test_data_)};
  //* Verify that acquired signature is correct:
  ASSERT_TRUE(sig_0.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_0.Verify(priv_key.publicKey(), test_data_));

  //* Production code:
  ECDSASignature<eECDSAEncoding::canonical> sig_1{std::move(sig_0)};
  //* Expectations:
  EXPECT_FALSE(sig_0.signature_ECDSA_SIG());  // NOLINT
  EXPECT_TRUE(sig_1.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_1.Verify(priv_key.publicKey(), test_data_));

  //* Production code:
  ECDSASignature<eECDSAEncoding::canonical> sig_2{std::move(sig_1)};
  //* Expectations:
  EXPECT_FALSE(sig_1.signature_ECDSA_SIG());  // NOLINT
  EXPECT_TRUE(sig_2.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_2.Verify(priv_key.publicKey(), test_data_));
}

TEST_F(ECDCSASignatureTest, test_moving_semantics_assign_operator)
{
  openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

  ECDSASignature<eECDSAEncoding::canonical> sig_0{
      ECDSASignature<eECDSAEncoding::canonical>::Sign(priv_key, test_data_)};
  //* Verify that acquired signature is correct:
  ASSERT_TRUE(sig_0.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_0.Verify(priv_key.publicKey(), test_data_));

  ECDSASignature<eECDSAEncoding::canonical> sig_1;
  //* Production code:
  sig_1 = std::move(sig_0);
  //* Expectations:
  EXPECT_FALSE(sig_0.signature_ECDSA_SIG());  // NOLINT
  EXPECT_TRUE(sig_1.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_1.Verify(priv_key.publicKey(), test_data_));

  ECDSASignature<eECDSAEncoding::canonical> sig_2;
  //* Production code:
  sig_2 = std::move(sig_1);
  //* Expectations:
  EXPECT_FALSE(sig_1.signature_ECDSA_SIG());  // NOLINT
  EXPECT_TRUE(sig_2.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_2.Verify(priv_key.publicKey(), test_data_));
}

TEST_F(ECDCSASignatureTest, test_copy_constructor)
{
  openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

  ECDSASignature<eECDSAEncoding::canonical> sig_0{
      ECDSASignature<eECDSAEncoding::canonical>::Sign(priv_key, test_data_)};
  //* Verify that acquired signature is correct:
  ASSERT_TRUE(sig_0.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_0.Verify(priv_key.publicKey(), test_data_));

  //* Production code:
  ECDSASignature<eECDSAEncoding::canonical> sig_1{sig_0};
  //* Expectations:
  EXPECT_TRUE(sig_0.signature_ECDSA_SIG());
  EXPECT_TRUE(sig_1.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_0.Verify(priv_key.publicKey(), test_data_));
  ASSERT_TRUE(sig_1.Verify(priv_key.publicKey(), test_data_));

  //* Production code:
  ECDSASignature<eECDSAEncoding::canonical> sig_2{sig_1};
  //* Expectations:
  EXPECT_TRUE(sig_1.signature_ECDSA_SIG());
  EXPECT_TRUE(sig_2.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_1.Verify(priv_key.publicKey(), test_data_));
  ASSERT_TRUE(sig_2.Verify(priv_key.publicKey(), test_data_));
}

TEST_F(ECDCSASignatureTest, test_copy_assign_operator)
{
  openssl::ECDSAPrivateKey<eECDSAEncoding::bin> priv_key(priv_key_data_);

  ECDSASignature<eECDSAEncoding::canonical> sig_0{
      ECDSASignature<eECDSAEncoding::canonical>::Sign(priv_key, test_data_)};
  //* Verify that acquired signature is correct:
  ASSERT_TRUE(sig_0.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_0.Verify(priv_key.publicKey(), test_data_));

  ECDSASignature<eECDSAEncoding::canonical> sig_1;
  //* Production code:
  sig_1 = sig_0;
  //* Expectations:
  EXPECT_TRUE(sig_0.signature_ECDSA_SIG());
  EXPECT_TRUE(sig_1.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_0.Verify(priv_key.publicKey(), test_data_));
  ASSERT_TRUE(sig_1.Verify(priv_key.publicKey(), test_data_));

  ECDSASignature<eECDSAEncoding::canonical> sig_2;
  //* Production code:
  sig_2 = sig_1;
  //* Expectations:
  EXPECT_TRUE(sig_1.signature_ECDSA_SIG());
  EXPECT_TRUE(sig_2.signature_ECDSA_SIG());
  ASSERT_TRUE(sig_1.Verify(priv_key.publicKey(), test_data_));
  ASSERT_TRUE(sig_2.Verify(priv_key.publicKey(), test_data_));
}

}  // namespace

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
