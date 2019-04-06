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

#include "crypto/ecdsa.hpp"

#include "gmock/gmock.h"

namespace fetch {
namespace crypto {

namespace {

class ECDSASignerVerifierTest : public testing::Test
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
};

TEST_F(ECDSASignerVerifierTest, test_sign_verify_cycle_with_predfined_private_key)
{
  ECDSASigner signer;
  signer.SetPrivateKey(priv_key_data_);

  ASSERT_TRUE(signer.Sign(test_data_));

  ECDSAVerifier verifier{signer.identity()};

  EXPECT_TRUE(verifier.Verify(test_data_, signer.signature()));
}

TEST_F(ECDSASignerVerifierTest, test_sign_verify_cycle_generated_key)
{
  ECDSASigner signer;
  signer.GenerateKeys();

  ASSERT_TRUE(signer.Sign(test_data_));

  ECDSAVerifier verifier{signer.identity()};

  EXPECT_TRUE(verifier.Verify(test_data_, signer.signature()));
}

TEST_F(ECDSASignerVerifierTest, test_sane_verify)
{
  ECDSASigner signer;
  signer.GenerateKeys();

  ASSERT_TRUE(signer.Sign(test_data_));

  ECDSAVerifier falseVerifier{Identity()};

  EXPECT_FALSE(falseVerifier);
  EXPECT_FALSE(falseVerifier.Verify(test_data_, signer.signature()));

  ECDSAVerifier trueVerifier{signer.identity()};

  EXPECT_TRUE(trueVerifier);
  EXPECT_TRUE(trueVerifier.Verify(test_data_, signer.signature()));
}

}  // namespace

}  // namespace crypto
}  // namespace fetch
