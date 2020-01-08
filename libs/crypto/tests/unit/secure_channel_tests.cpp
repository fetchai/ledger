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
#include "crypto/ecdsa.hpp"
#include "crypto/secure_channel.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::crypto::ECDSASigner;
using fetch::byte_array::ConstByteArray;
using fetch::crypto::SecureChannel;

class SecureChannelTests : public ::testing::Test
{
protected:
  ECDSASigner    alice_private_key_;
  ConstByteArray alice_pubic_key_{alice_private_key_.identity().identifier()};
  SecureChannel  alice_channel_{alice_private_key_};
  ECDSASigner    bob_private_key_;
  ConstByteArray bob_pubic_key_{bob_private_key_.identity().identifier()};
  SecureChannel  bob_channel_{bob_private_key_};
};

TEST_F(SecureChannelTests, CheckAliceSendsToBob)
{
  static const uint16_t SERVICE = 200;
  static const uint16_t CHANNEL = 202;
  static const uint16_t COUNTER = 304;

  ConstByteArray msg = "Hello Bob, this is a message from Alice";

  ConstByteArray payload{};
  ASSERT_TRUE(alice_channel_.Encrypt(bob_pubic_key_, SERVICE, CHANNEL, COUNTER, msg, payload));

  ConstByteArray recovered{};
  ASSERT_TRUE(
      bob_channel_.Decrypt(alice_pubic_key_, SERVICE, CHANNEL, COUNTER, payload, recovered));

  ASSERT_EQ(msg, recovered);
}

TEST_F(SecureChannelTests, CheckAliceSendsToBobMultipleOfBlockSize)
{
  static const uint16_t SERVICE = 200;
  static const uint16_t CHANNEL = 202;
  static const uint16_t COUNTER = 304;

  ConstByteArray msg =
      "Hello Bob, this is a message from Alice.....just aligning message to multiple of AES block "
      "size!";

  ConstByteArray payload{};
  ASSERT_TRUE(alice_channel_.Encrypt(bob_pubic_key_, SERVICE, CHANNEL, COUNTER, msg, payload));

  ConstByteArray recovered{};
  ASSERT_TRUE(
      bob_channel_.Decrypt(alice_pubic_key_, SERVICE, CHANNEL, COUNTER, payload, recovered));

  ASSERT_EQ(msg, recovered);
}

}  // namespace
