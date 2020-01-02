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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/ecdh.hpp"
#include "crypto/ecdsa.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::crypto::ECDSASigner;
using fetch::crypto::ECDSAVerifier;
using fetch::byte_array::ConstByteArray;
using fetch::crypto::ComputeSharedKey;

class EcdhTests : public ::testing::Test
{
protected:
  ECDSASigner   alice_private_key_;
  ECDSAVerifier alice_public_key_{alice_private_key_.identity()};
  ECDSASigner   bob_private_key_;
  ECDSAVerifier bob_public_key_{bob_private_key_.identity()};
};

TEST_F(EcdhTests, BasicCheck)
{
  ConstByteArray alice_shared_key{};
  ASSERT_TRUE(ComputeSharedKey(alice_private_key_, bob_public_key_, alice_shared_key));

  ConstByteArray bob_shared_key{};
  ASSERT_TRUE(ComputeSharedKey(bob_private_key_, alice_public_key_, bob_shared_key));

  ASSERT_EQ(alice_shared_key, bob_shared_key);
}

}  // namespace
