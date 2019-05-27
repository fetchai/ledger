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

#include "core/byte_array/decoders.hpp"
#include "ledger/chain/transaction_encoding.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::byte_array::FromHex;
using fetch::serializers::ByteArrayBuffer;
using fetch::ledger::detail::EncodeInteger;
using fetch::ledger::detail::DecodeInteger;

ByteArrayBuffer CreateEncodedBuffer(char const *hex)
{
  return {FromHex(hex)};
}

TEST(IntegerEncodingTests, CheckSmallUnsignedEncode)
{
  auto const encoded = EncodeInteger(4);
  EXPECT_EQ(encoded.ToHex(), std::string{"04"});
}

TEST(IntegerEncodingTests, CheckSmallSignedEncode)
{
  auto const encoded = EncodeInteger(-4);
  EXPECT_EQ(encoded.ToHex(), std::string{"e4"});
}

TEST(IntegerEncodingTests, Check1ByteUnsignedEncode)
{
  auto const encoded = EncodeInteger(0x80);
  EXPECT_EQ(encoded.ToHex(), std::string{"c080"});
}

TEST(IntegerEncodingTests, Check2ByteUnsignedEncode)
{
  auto const encoded = EncodeInteger(0xEDEF);
  EXPECT_EQ(encoded.ToHex(), std::string{"c1edef"});
}

TEST(IntegerEncodingTests, Check4ByteUnsignedEncode)
{
  auto const encoded = EncodeInteger(0xEDEFABCDu);
  EXPECT_EQ(encoded.ToHex(), std::string{"c2edefabcd"});
}

TEST(IntegerEncodingTests, Check8ByteUnsignedEncode)
{
  auto const encoded = EncodeInteger(0xEDEFABCD01234567ull);
  EXPECT_EQ(encoded.ToHex(), std::string{"c3edefabcd01234567"});
}

TEST(IntegerEncodingTests, Check1ByteSignedEncode)
{
  auto const encoded = EncodeInteger(-0x80);
  EXPECT_EQ(encoded.ToHex(), std::string{"d080"});
}

TEST(IntegerEncodingTests, Check2ByteSignedEncode)
{
  auto const encoded = EncodeInteger(-0xEDEF);
  EXPECT_EQ(encoded.ToHex(), std::string{"d1edef"});
}

TEST(IntegerEncodingTests, Check4ByteSignedEncode)
{
  auto const encoded = EncodeInteger(-0xEDEFABCDll);
  EXPECT_EQ(encoded.ToHex(), std::string{"d2edefabcd"});
}

TEST(IntegerEncodingTests, Check8ByteSignedEncode)
{
  auto const encoded = EncodeInteger(-0x6DEFABCD01234567ll);
  EXPECT_EQ(encoded.ToHex(), std::string{"d36defabcd01234567"});
}

TEST(IntegerEncodingTests, CheckSmallUnsignedDecode)
{
  auto buffer = CreateEncodedBuffer("04");
  EXPECT_EQ(DecodeInteger<uint32_t>(buffer), 4u);
}

TEST(IntegerEncodingTests, CheckSmallSignedDecode)
{
  auto buffer = CreateEncodedBuffer("E4");
  EXPECT_EQ(DecodeInteger<int32_t>(buffer), -4);
}

TEST(IntegerEncodingTests, Check1ByteUnsignedDecode)
{
  auto buffer = CreateEncodedBuffer("C080");
  EXPECT_EQ(DecodeInteger<uint32_t>(buffer), 0x80u);
}

TEST(IntegerEncodingTests, Check2ByteUnsignedDecode)
{
  auto buffer = CreateEncodedBuffer("C1EDEF");
  EXPECT_EQ(DecodeInteger<uint32_t>(buffer), 0xEDEFu);
}

TEST(IntegerEncodingTests, Check4ByteUnsignedDecode)
{
  auto buffer = CreateEncodedBuffer("C2EDEFABCD");
  EXPECT_EQ(DecodeInteger<uint32_t>(buffer), 0xEDEFABCDu);
}

TEST(IntegerEncodingTests, Check8ByteUnsignedDecode)
{
  auto buffer = CreateEncodedBuffer("C3EDEFABCD01234567");
  EXPECT_EQ(DecodeInteger<uint64_t>(buffer), 0xEDEFABCD01234567ull);
}

TEST(IntegerEncodingTests, Check1ByteSignedDecode)
{
  auto buffer = CreateEncodedBuffer("D080");
  EXPECT_EQ(DecodeInteger<int32_t>(buffer), -0x80);
}

TEST(IntegerEncodingTests, Check2ByteSignedDecode)
{
  auto buffer = CreateEncodedBuffer("D1EDEF");
  EXPECT_EQ(DecodeInteger<int32_t>(buffer), -0xEDEF);
}

TEST(IntegerEncodingTests, Check4ByteSignedDecode)
{
  auto buffer = CreateEncodedBuffer("D2EDEFABCD");
  EXPECT_EQ(DecodeInteger<int64_t>(buffer), -0xEDEFABCDll);
}

TEST(IntegerEncodingTests, Check8ByteSignedDecode)
{
  auto buffer = CreateEncodedBuffer("D36DEFABCD01234567");
  EXPECT_EQ(DecodeInteger<int64_t>(buffer), -0x6DEFABCD01234567ll);
}

TEST(IntegerEncodingTests, CheckFailure)
{
  auto buffer = CreateEncodedBuffer("C200FFFFFF");
  EXPECT_EQ(DecodeInteger<uint32_t>(buffer), 0xFFFFFFu);
}

} // namespace
