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

#include "core/byte_array/encoders.hpp"
#include "crypto/openssl_ecdsa_private_key.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

class ECDCSAPrivateKeyTest : public testing::Test
{
protected:
  const fetch::byte_array::ConstByteArray priv_key_data__bin_ = {
      0x92, 0xad, 0x61, 0xcf, 0xfc, 0xb9, 0x2a, 0x17, 0x02, 0xa3, 0xd6,
      0x03, 0xa0, 0x0d, 0x6e, 0xb3, 0xad, 0x92, 0x0f, 0x8c, 0xec, 0x43,
      0xda, 0x41, 0x8f, 0x01, 0x04, 0xc6, 0xc6, 0xc9, 0xe0, 0x5e};

  const std::string priv_key_hex_str__bin_ =
      "92ad61cffcb92a1702a3d603a00d6eb3ad920f8cec43da418f0104c6c6c9e05e";

  const fetch::byte_array::ConstByteArray public_key_data__bin_ = {
      0x04, 0x55, 0x5a, 0x38, 0xa4, 0x2d, 0xb2, 0x9d, 0x05, 0xcd, 0xe3, 0xea, 0xa0,
      0x93, 0x07, 0x89, 0x46, 0x16, 0xb5, 0xa2, 0xb5, 0xa3, 0x02, 0xe9, 0x13, 0xee,
      0xa2, 0x6d, 0x03, 0x48, 0xec, 0x5b, 0x5c, 0x07, 0x30, 0x2d, 0xfc, 0xdb, 0xd5,
      0xcd, 0xa1, 0x73, 0x74, 0xcd, 0x2f, 0x6b, 0xec, 0xcf, 0xc4, 0x67, 0xa1, 0x51,
      0x3a, 0xa1, 0xf7, 0xb4, 0xeb, 0x3f, 0x1c, 0x00, 0x6b, 0x6e, 0xb6, 0x2b, 0x51};
  void SetUp()
  {}

  void TearDown()
  {}
};

TEST_F(ECDCSAPrivateKeyTest, test_instantiation_of_private_key_gives_corect_public_key__bin)
{
  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::bin> x(priv_key_data__bin_);

  //* Expectations:
  EXPECT_TRUE(x.key());
  EXPECT_TRUE(x.publicKey().key());

  EXPECT_EQ(priv_key_data__bin_, x.KeyAsBin());
  EXPECT_EQ(public_key_data__bin_, x.publicKey().keyAsBin());
}

// TODO(issue 36): A bit lame test, needs to be tested rather with & against hardcoded DER
// encoded data
TEST_F(ECDCSAPrivateKeyTest, test_instantiation_of_private_key_gives_corect_public_key__DER)
{
  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::bin> x(priv_key_data__bin_);

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
  // TODO(issue 36): Public key does not support `DER` enc. yet so it defaults to `bin`
  // enc. in when set to
  // DER. ASSERT_NE(x.publicKey().KeyAsBin(), x_der.publicKey().KeyAsBin());

  //* Conv. binary data back from DER to bin encoding
  ECDSAPrivateKey<eECDSAEncoding::bin> x_2(x_der);

  //* Expectations:
  EXPECT_EQ(priv_key_data__bin_, x_2.KeyAsBin());
  EXPECT_EQ(public_key_data__bin_, x_2.publicKey().keyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, test_convert_from_bin_to_canonical)
{
  //* Production code:
  ECDSAPrivateKey<eECDSAEncoding::bin> x(priv_key_data__bin_);

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
  //* bin & canonical encodings DIFFER for PUBLIC Key (0x04 z component at the
  // beginning)
  ASSERT_NE(x.publicKey().keyAsBin(), x_can.publicKey().keyAsBin());

  //* Converting back to original bin encoding
  ECDSAPrivateKey<eECDSAEncoding::bin> x_bin_2(x_can);

  //* Mandatory validity checks:
  ASSERT_TRUE(x_bin_2.key());
  ASSERT_TRUE(x_bin_2.publicKey().key());
  ASSERT_EQ(x.key(), x_bin_2.key());
  ASSERT_EQ(x.publicKey().key(), x_bin_2.publicKey().key());

  EXPECT_EQ(priv_key_data__bin_, x_bin_2.KeyAsBin());
  EXPECT_EQ(public_key_data__bin_, x_bin_2.publicKey().keyAsBin());
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
  ECDSAPrivateKey<> x(priv_key_data__bin_);

  //* Expectations:
  EXPECT_TRUE(x.key());
  EXPECT_EQ(priv_key_data__bin_, x.KeyAsBin());
}

TEST_F(ECDCSAPrivateKeyTest, public_key_conversion_cycle)
{
  for (std::size_t i = 0; i < 100; ++i)
  {
    //* Generating priv & pub key pair
    ECDSAPrivateKey<> const priv_key;

    //* Production code:
    auto const                          serialized_pub_key = priv_key.publicKey().keyAsBin();
    decltype(priv_key)::public_key_type pub_key{serialized_pub_key};

    //* Expectations:
    EXPECT_EQ(ECDSAPrivateKey<>::ecdsa_curve_type::publicKeySize, serialized_pub_key.size());
    EXPECT_EQ(priv_key.publicKey().keyAsBin(), pub_key.keyAsBin());
  }
}

// TODO(issue 36): Add more tests

}  // namespace

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
