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

#include "crypto/openssl_context_detail.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace context {
namespace detail {

namespace {

class SessionPrimitiveTest : public ::testing::Test
{
protected:
  virtual void SetUp()
  {}

  virtual void TearDown()
  {}
};

TEST_F(SessionPrimitiveTest, test_BN_CTX_start)
{
  EXPECT_EQ(SessionPrimitive<BN_CTX>::start, &BN_CTX_start);
}

TEST_F(SessionPrimitiveTest, test_BN_CTX_end)
{
  EXPECT_EQ(SessionPrimitive<BN_CTX>::end, &BN_CTX_end);
}

}  // namespace

}  // namespace detail
}  // namespace context
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
