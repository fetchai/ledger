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

#include "crypto/openssl_memory_detail.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {
namespace detail {

namespace {

class DeleterPrimitiveTest : public testing::Test
{
protected:
  virtual void SetUp()
  {}

  virtual void TearDown()
  {}
};

TEST_F(DeleterPrimitiveTest, test_BIGNUM_free)
{
  EXPECT_EQ(DeleterPrimitive<BIGNUM>::function, &BN_free);
}

TEST_F(DeleterPrimitiveTest, test_BIGNUM_clear_free)
{
  EXPECT_EQ((DeleterPrimitive<BIGNUM, eDeleteStrategy::clearing>::function), &BN_clear_free);
}

TEST_F(DeleterPrimitiveTest, test_BN_CTX_free)
{
  EXPECT_EQ(DeleterPrimitive<BN_CTX>::function, &BN_CTX_free);
}

TEST_F(DeleterPrimitiveTest, test_EC_POINT_free)
{
  EXPECT_EQ(DeleterPrimitive<EC_POINT>::function, &EC_POINT_free);
}

TEST_F(DeleterPrimitiveTest, test_EC_POINT_clear_free)
{
  EXPECT_EQ((DeleterPrimitive<EC_POINT, eDeleteStrategy::clearing>::function),
            &EC_POINT_clear_free);
}

TEST_F(DeleterPrimitiveTest, test_EC_KEY_free)
{
  EXPECT_EQ(DeleterPrimitive<EC_KEY>::function, &EC_KEY_free);
}

TEST_F(DeleterPrimitiveTest, test_EC_GROUP_free)
{
  EXPECT_EQ(DeleterPrimitive<EC_GROUP>::function, &EC_GROUP_free);
}

TEST_F(DeleterPrimitiveTest, test_EC_GROUP_clear_free)
{
  EXPECT_EQ((DeleterPrimitive<EC_GROUP, eDeleteStrategy::clearing>::function),
            &EC_GROUP_clear_free);
}

TEST_F(DeleterPrimitiveTest, test_ECDSA_SIG_clear_free)
{
  EXPECT_EQ((DeleterPrimitive<ECDSA_SIG>::function), &ECDSA_SIG_free);
}

}  // namespace

}  // namespace detail
}  // namespace memory
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
