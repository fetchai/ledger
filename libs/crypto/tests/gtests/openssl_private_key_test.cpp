#include "core/byte_array/encoders.hpp"
#include "crypto/openssl_ecdsa_private_key.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

using ::testing::StrictMock;
using ::testing::Return;

class ECDCSAPrivateKeyTest : public testing::Test
{
protected:
  const fetch::byte_array::ConstByteArray priv_key_data__bin = {
      0x92, 0xad, 0x61, 0xcf, 0xfc, 0xb9, 0x2a, 0x17, 0x02, 0xa3, 0xd6,
      0x03, 0xa0, 0x0d, 0x6e, 0xb3, 0xad, 0x92, 0x0f, 0x8c, 0xec, 0x43,
      0xda, 0x41, 0x8f, 0x01, 0x04, 0xc6, 0xc6, 0xc9, 0xe0, 0x5e};

  const std::string priv_key_hex_str__bin =
      "92ad61cffcb92a1702a3d603a00d6eb3ad920f8cec43da418f0104c6c6c9e05e";

  const fetch::byte_array::ConstByteArray public_key_data__bin = {
      0x04, 0x55, 0x5a, 0x38, 0xa4, 0x2d, 0xb2, 0x9d, 0x05, 0xcd, 0xe3, 0xea, 0xa0,
      0x93, 0x07, 0x89, 0x46, 0x16, 0xb5, 0xa2, 0xb5, 0xa3, 0x02, 0xe9, 0x13, 0xee,
      0xa2, 0x6d, 0x03, 0x48, 0xec, 0x5b, 0x5c, 0x07, 0x30, 0x2d, 0xfc, 0xdb, 0xd5,
      0xcd, 0xa1, 0x73, 0x74, 0xcd, 0x2f, 0x6b, 0xec, 0xcf, 0xc4, 0x67, 0xa1, 0x51,
      0x3a, 0xa1, 0xf7, 0xb4, 0xeb, 0x3f, 0x1c, 0x00, 0x6b, 0x6e, 0xb6, 0x2b, 0x51};
  void SetUp() {}

  void TearDown() {}
};

TEST_F(ECDCSAPrivateKeyTest, test_instantiation_of_private_key_gives_corect_public_key__bin)
{
  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::bin> x(priv_key_data__bin);

  //* Expectations:
  EXPECT_TRUE(x.key());
  EXPECT_TRUE(x.publicKey().key());

  EXPECT_EQ(priv_key_data__bin, x.KeyAsBin());
  EXPECT_EQ(public_key_data__bin, x.publicKey().keyAsBin());
}

// TODO: A bit lame test, needs to be tesetd rather with & against hardcoded DER encoded data
TEST_F(ECDCSAPrivateKeyTest, test_instantiation_of_private_key_gives_corect_public_key__DER)
{
  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::bin> x(priv_key_data__bin);

  //* Mandatory validity checks:
  ASSERT_TRUE(x.key());
  ASSERT_TRUE(x.publicKey().key());

  //* Conv. binary data from bin to DER encoding
  ECDSAPrivateKey<eECDSAEncoding::DER> x_der(x);

  //* Mandatory validity checks:
  ASSERT_TRUE(x_der.key());
  ASSERT_TRUE(x_der.publicKey().key());
  ASSERT_EQ(x.key(), x_der.key());
  ASSERT_NE(x.KeyAsBin(), x_der.KeyAsBin());
  ASSERT_EQ(x.publicKey().key(), x_der.publicKey().key());
  // TODO: Public key does not support `DER` enc. yet so it defaults to `bin` enc. in when set to
  // DER. ASSERT_NE(x.publicKey().KeyAsBin(), x_der.publicKey().KeyAsBin());

  //* Conv. binary data back from DER to bin encoding
  ECDSAPrivateKey<eECDSAEncoding::bin> x_2(x_der);

  //* Expectations:
  EXPECT_EQ(priv_key_data__bin, x_2.KeyAsBin());
  EXPECT_EQ(public_key_data__bin, x_2.publicKey().keyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_convert_from_bin_to_canonical)
{
  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::bin> x(priv_key_data__bin);

  //* Mandatory validity checks:
  ASSERT_TRUE(x.key());
  ASSERT_TRUE(x.publicKey().key());

  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::canonical> x_can(x);

  //* Mandatory validity checks:
  ASSERT_TRUE(x_can.key());
  ASSERT_TRUE(x_can.publicKey().key());
  ASSERT_EQ(x.key(), x_can.key());
  //* bin & canonical encodings are the same for PRIVATE Key
  ASSERT_EQ(x.KeyAsBin(), x_can.KeyAsBin());
  ASSERT_EQ(x.publicKey().key(), x_can.publicKey().key());
  //* bin & canonical encodings DIFFER for PUBLIC Key (0x04 z component at the beginning)
  ASSERT_NE(x.publicKey().keyAsBin(), x_can.publicKey().keyAsBin());

  //* Converting back to original bin encoding
  ECDSAPrivateKey<eECDSAEncoding::bin> x_bin_2(x_can);

  //* Mandatory validity checks:
  ASSERT_TRUE(x_bin_2.key());
  ASSERT_TRUE(x_bin_2.publicKey().key());
  ASSERT_EQ(x.key(), x_bin_2.key());
  ASSERT_EQ(x.publicKey().key(), x_bin_2.publicKey().key());

  EXPECT_EQ(priv_key_data__bin, x_bin_2.KeyAsBin());
  EXPECT_EQ(public_key_data__bin, x_bin_2.publicKey().keyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_each_generated_key_is_different)
{
  //* Production code:
  ECDSAPrivateKey<> x;
  ECDSAPrivateKey<> y;

  //* Expectations:
  EXPECT_TRUE(x.key());
  EXPECT_TRUE(x.publicKey().keyAsEC_POINT());

  EXPECT_TRUE(y.key());
  EXPECT_TRUE(y.publicKey().keyAsEC_POINT());

  EXPECT_NE(x.KeyAsBin(), y.KeyAsBin());
  EXPECT_NE(x.publicKey().keyAsBin(), y.publicKey().keyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_key_conversion_to_byte_array)
{
  //* Production code:
  ECDSAPrivateKey<> x(priv_key_data__bin);

  //* Expectations:
  EXPECT_TRUE(x.key());
  EXPECT_EQ(priv_key_data__bin, x.KeyAsBin());
}

// TODO: Add more tests

}  // namespace

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
