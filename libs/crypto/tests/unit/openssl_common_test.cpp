//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "crypto/openssl_common.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

class ECDSACurveTest : public testing::Test
{
protected:
  template <int P_ECDSA_Curve_NID>
  void test_ECDSACurve(uint8_t const expected_sn, std::size_t const expected_privateKeySize,
                       std::size_t const expected_publicKeySize,
                       std::size_t const expected_signatureSize)
  {
    using EcdsaCurveType = ECDSACurve<P_ECDSA_Curve_NID>;
    EXPECT_EQ(EcdsaCurveType::nid, P_ECDSA_Curve_NID);
    ASSERT_EQ(expected_sn, EcdsaCurveType::sn);
    EXPECT_EQ(expected_privateKeySize, EcdsaCurveType::privateKeySize);
    EXPECT_EQ(expected_publicKeySize, EcdsaCurveType::publicKeySize);
    EXPECT_EQ(expected_signatureSize, EcdsaCurveType::signatureSize);
  }
};

TEST_F(ECDSACurveTest, test_ECDSACurve_for_NID_secp256k1)
{
  test_ECDSACurve<NID_secp256k1>(fetch::crypto::SignatureType::SECP256K1_UNCOMPRESSED, 32, 64, 64);
}

class ECDSAAffineCoordinatesConversionTest : public testing::Test
{
protected:
  void test_convert_canonical_with_padding(SharedPointerType<BIGNUM const> const &x,
                                           SharedPointerType<BIGNUM const> const &y)
  {
    ASSERT_GT(ECDSAAffineCoordinatesConversion<>::x_size,
              static_cast<std::size_t>(BN_num_bytes(x.get())));
    ASSERT_GT(ECDSAAffineCoordinatesConversion<>::y_size,
              static_cast<std::size_t>(BN_num_bytes(y.get())));

    auto serialized_to_ba = ECDSAAffineCoordinatesConversion<>::Convert2Canonical(x.get(), y.get());
    EXPECT_EQ(ECDSAAffineCoordinatesConversion<>::EcdsaCurveType::publicKeySize,
              serialized_to_ba.size());

    SharedPointerType<BIGNUM> x2{BN_new()};
    SharedPointerType<BIGNUM> y2{BN_new()};

    ECDSAAffineCoordinatesConversion<>::ConvertFromCanonical(serialized_to_ba, x2.get(), y2.get());

    EXPECT_EQ(0, BN_cmp(x.get(), x2.get()));
    EXPECT_EQ(0, BN_cmp(y.get(), y2.get()));
  }
};

TEST_F(ECDSAAffineCoordinatesConversionTest, test_convert_canonical_with_padding)
{
  SharedPointerType<BIGNUM> x{BN_new()};
  SharedPointerType<BIGNUM> y{BN_new()};

  byte_array::ConstByteArray const x_ba({1, 2, 3, 4, 5});
  byte_array::ConstByteArray const y_ba({6, 7, 8, 9, 10});

  ASSERT_NE(x_ba, y_ba);

  ASSERT_NE(nullptr, BN_bin2bn(x_ba.pointer(), static_cast<int>(x_ba.size()), x.get()));
  ASSERT_NE(nullptr, BN_bin2bn(y_ba.pointer(), static_cast<int>(y_ba.size()), y.get()));

  ASSERT_NE(0, BN_cmp(x.get(), y.get()));

  test_convert_canonical_with_padding(x, y);
}

TEST_F(ECDSAAffineCoordinatesConversionTest, test_convert_canonical_with_padding_random)
{
  for (std::size_t j = 0; j < 100; ++j)
  {
    SharedPointerType<BIGNUM> x{BN_new()};
    SharedPointerType<BIGNUM> y{BN_new()};

    constexpr int bn_size_in_bites = 8 * 5;

    ASSERT_EQ(1, BN_rand(x.get(), bn_size_in_bites, -1, 0));
    std::size_t i = 0;
    //* Ensuring that number are different (probability that this loop cycles more than once is
    //(almost) zero)
    do
    {
      ASSERT_EQ(1, BN_rand(y.get(), bn_size_in_bites, -1, 0));
    } while (0 == BN_cmp(x.get(), y.get()) && i++ < 100);
    ASSERT_NE(0, BN_cmp(x.get(), y.get()));

    test_convert_canonical_with_padding(x, y);
  }
}

}  // namespace

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
