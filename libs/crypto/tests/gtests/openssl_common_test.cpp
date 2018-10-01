//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

class ECDSACurveTest : public testing::Test
{
protected:
  virtual void SetUp()
  {}

  virtual void TearDown()
  {}

  template <int P_ECDSA_Curve_NID>
  void test_ECDSACurve(const char *const expected_sn, const std::size_t expected_privateKeySize,
                       const std::size_t expected_publicKeySize,
                       const std::size_t expected_signatureSize)
  {
    using ecdsa_curve_type = ECDSACurve<P_ECDSA_Curve_NID>;
    EXPECT_EQ(ecdsa_curve_type::nid, P_ECDSA_Curve_NID);
    ASSERT_STREQ(expected_sn, ecdsa_curve_type::sn);
    EXPECT_EQ(expected_privateKeySize, ecdsa_curve_type::privateKeySize);
    EXPECT_EQ(expected_publicKeySize, ecdsa_curve_type::publicKeySize);
    EXPECT_EQ(expected_signatureSize, ecdsa_curve_type::signatureSize);
  }
};

TEST_F(ECDSACurveTest, test_ECDSACurve_for_NID_secp256k1)
{
  test_ECDSACurve<NID_secp256k1>(SN_secp256k1, 32, 64, 64);
}

}  // namespace

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
