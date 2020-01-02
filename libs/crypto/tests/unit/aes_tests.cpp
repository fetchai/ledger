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

#include "aes.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/block_cipher.hpp"

#include "gtest/gtest.h"

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromHex;
using fetch::crypto::AesBlockCipher;
using fetch::crypto::BlockCipher;

namespace {

TEST(AesTests, BasicAes256CbcTest)
{
  // define the inputs to the object
  ConstByteArray plain_text = "The quick brown fox jumps over the lazy dog";
  ConstByteArray key = FromHex("0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20");
  ConstByteArray iv  = FromHex("0102030405060708090A0B0C0D0E0F10");

  // encrypt the plain text
  ConstByteArray cipher_text{};
  ASSERT_TRUE(AesBlockCipher::Encrypt(BlockCipher::AES_256_CBC, key, iv, plain_text, cipher_text));

  ConstByteArray recovered_text{};
  ASSERT_TRUE(
      AesBlockCipher::Decrypt(BlockCipher::AES_256_CBC, key, iv, cipher_text, recovered_text));

  ASSERT_EQ(plain_text, recovered_text);
}

TEST(AesTests, ExactMultipleOfBlockSize)
{
  // define the inputs to the object
  ConstByteArray plain_text = "The quick brown fox jumps over the lazy dog.....";
  ConstByteArray key = FromHex("0102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F20");
  ConstByteArray iv  = FromHex("0102030405060708090A0B0C0D0E0F10");

  // check the message size is as expected
  ASSERT_EQ(plain_text.size() % 16, 0);

  // encrypt the plain text
  ConstByteArray cipher_text{};
  ASSERT_TRUE(AesBlockCipher::Encrypt(BlockCipher::AES_256_CBC, key, iv, plain_text, cipher_text));

  ConstByteArray recovered_text{};
  ASSERT_TRUE(
      AesBlockCipher::Decrypt(BlockCipher::AES_256_CBC, key, iv, cipher_text, recovered_text));

  ASSERT_EQ(plain_text, recovered_text);
}

}  // namespace
