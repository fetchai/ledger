#include "crypto/ecdsa_sign.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

class ECDCSASignTest : public testing::Test
{
protected:
  const fetch::byte_array::ConstByteArray priv_key_data = {
      0x92, 0xad, 0x61, 0xcf, 0xfc, 0xb9, 0x2a, 0x17, 0x02, 0xa3, 0xd6,
      0x03, 0xa0, 0x0d, 0x6e, 0xb3, 0xad, 0x92, 0x0f, 0x8c, 0xec, 0x43,
      0xda, 0x41, 0x8f, 0x01, 0x04, 0xc6, 0xc6, 0xc9, 0xe0, 0x5e};

  const fetch::byte_array::ConstByteArray test_data = {
      0x2a, 0xc8, 0xa5, 0xb0, 0x45, 0xfc, 0x3e, 0xa4, 0xaf, 0x70, 0xf7, 0x34,
      0xaa, 0xda, 0x83, 0xe5, 0x0b, 0x16, 0xff, 0x16, 0x73, 0x62, 0x27, 0xf3,
      0xf9, 0xe9, 0x2b, 0xdd, 0x3a, 0x1d, 0xdc, 0x42, 0x01, 0xaa, 0x05};

  void SetUp() {}

  void TearDown() {}
};

TEST_F(ECDCSASignTest, test_sign_verify_cycle)
{
  //* Production code:
  openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

  const byte_array::ConstByteArray signature{ecdsa_sign(priv_key, test_data)};

  const auto verification_result{
      ecdsa_verify(priv_key.publicKey(), test_data, signature)};

  //* Expectations:
  EXPECT_TRUE(verification_result);
}

TEST_F(ECDCSASignTest, test_wrong_signature_fails_to_verify)
{
  //* Production code:
  openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

  byte_array::ByteArray signature{ecdsa_sign(priv_key, test_data)};

  ASSERT_TRUE(signature.size() > 0);

  //* Modify the correct signature to invalidate it
  signature[signature.size() - 1] += 1;

  const auto verification_result{
      ecdsa_verify(priv_key.publicKey(), test_data, signature)};

  //* Expectations:
  EXPECT_FALSE(verification_result);
}

TEST_F(ECDCSASignTest, test_invalid_signature_causes_exception)
{
  //* Production code:
  openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

  byte_array::ByteArray signature{ecdsa_sign(priv_key, test_data)};

  //* Verify that acquired signature is correct:
  ASSERT_TRUE(ecdsa_verify(priv_key.publicKey(), test_data, signature));

  //* Invalidating signature by modifying it's first byte of DER format
  ASSERT_TRUE(signature.size() > 0);
  signature[0] += 1;

  bool exception_has_been_thrown = false;
  try
  {
    ecdsa_verify(priv_key.publicKey(), test_data, signature);
  }
  catch (const std::runtime_error &ex)
  {
    exception_has_been_thrown = true;
  }

  //* Expectations:
  EXPECT_TRUE(exception_has_been_thrown);
}

TEST_F(ECDCSASignTest, test_wrong_data_fails_to_verify)
{
  //* Production code:
  openssl::ECDSAPrivateKey<> priv_key(priv_key_data);

  byte_array::ByteArray signature{ecdsa_sign(priv_key, test_data)};

  byte_array::ByteArray modified_data = test_data.Copy();
  ASSERT_TRUE(modified_data.size() > 0);

  //* Modify original data
  modified_data[0] += 1;

  const auto verification_result{
      ecdsa_verify(priv_key.publicKey(), modified_data, signature)};

  //* Expectations:
  EXPECT_FALSE(verification_result);
}

}  // namespace

}  // namespace crypto
}  // namespace fetch
