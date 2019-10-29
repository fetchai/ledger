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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

#include "gtest/gtest.h"

using namespace fetch::byte_array;

TEST(core_encode_decode_gtest, Hex_encode_decode_self_consistentcy)
{
  ByteArray str = "hello WoRld";
  EXPECT_EQ(FromHex(ToHex(str)), str);
  EXPECT_EQ(FromHex(ToHex(str.SubArray(3, 5))), str.SubArray(3, 5));
  EXPECT_EQ(FromHex(ToHex(str.SubArray(3, 5))), str.SubArray(3, 5).Copy());
  EXPECT_EQ(FromHex(ToHex(str)), "hello WoRld");
}

TEST(core_encode_decode_gtest, Hex_encoding_external_consistency)
{
  EXPECT_EQ(ToHex("Hello world"), "48656c6c6f20776f726c64");
  EXPECT_EQ(ToHex("Hello cahrs!@#$%^&*()_+"), "48656c6c6f20636168727321402324255e262a28295f2b");
}

TEST(core_encode_decode_gtest, some_simple_cases_for_hex)
{
  EXPECT_EQ(FromHex(ToHex("")), "");
  EXPECT_EQ(FromHex(ToHex("a")), "a");
  EXPECT_EQ(FromHex(ToHex("ab")), "ab");
  EXPECT_EQ(FromHex(ToHex("abc")), "abc");
  EXPECT_EQ(FromHex(ToHex("abcd")), "abcd");
}

TEST(core_encode_decode_gtest, Base64_encode_decode_self_consistentcy)
{
  ByteArray str = "hello WoRld";
  EXPECT_EQ(FromBase64(ToBase64(str)), str);
  EXPECT_EQ(FromBase64(ToBase64(str.SubArray(3, 5))), str.SubArray(3, 5));
  EXPECT_EQ(FromBase64(ToBase64(str.SubArray(3, 5))), str.SubArray(3, 5).Copy());
  EXPECT_EQ(FromBase64(ToBase64(str)), "hello WoRld");
}

TEST(core_encode_decode_gtest, Base64_encoding_consistency_with_Python)
{
  EXPECT_EQ(ToBase64("Hello world"), "SGVsbG8gd29ybGQ=");
  EXPECT_EQ(ToBase64("Hello cahrs!@#$%^&*()_+"), "SGVsbG8gY2FocnMhQCMkJV4mKigpXys=");
}

TEST(core_encode_decode_gtest, Base64_pad_testing)
{
  EXPECT_EQ(ToBase64("any carnal pleasure."), "YW55IGNhcm5hbCBwbGVhc3VyZS4=");
  EXPECT_EQ(ToBase64("any carnal pleasure"), "YW55IGNhcm5hbCBwbGVhc3VyZQ==");
  EXPECT_EQ(ToBase64("any carnal pleasur"), "YW55IGNhcm5hbCBwbGVhc3Vy");
  EXPECT_EQ(ToBase64("any carnal pleasu"), "YW55IGNhcm5hbCBwbGVhc3U=");
  EXPECT_EQ(ToBase64("any carnal pleas"), "YW55IGNhcm5hbCBwbGVhcw==");
}

TEST(core_encode_decode_gtest, some_simple_cases_for_base_64)
{
  EXPECT_EQ(FromBase64(ToBase64("")), "");
  EXPECT_EQ(FromBase64(ToBase64("a")), "a");
  EXPECT_EQ(FromBase64(ToBase64("ab")), "ab");
  EXPECT_EQ(FromBase64(ToBase64("abc")), "abc");
  EXPECT_EQ(FromBase64(ToBase64("abcd")), "abcd");
}
