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

#include "core/byte_array/encoders.hpp"
#include "crypto/fnv.hpp"
#include "crypto/hash.hpp"
#include "crypto/md5.hpp"
#include "crypto/sha1.hpp"
#include "crypto/sha256.hpp"
#include "crypto/sha512.hpp"

#include "gmock/gmock.h"

#include <cstddef>
#include <string>

namespace {

using namespace fetch;
using namespace fetch::crypto;

using byte_array_type = byte_array::ByteArray;

enum class Hasher
{
  MD5,
  SHA2_256,
  SHA2_512,
  SHA1,
  FNV
};

class HasherTestParam
{
public:
  HasherTestParam(Hasher type_, std::string output_empty_)
    : type{type_}
    , output_empty{output_empty_}
  {}

  Hasher      type;
  std::string output_empty;
  //  const std::string input1;
  //  const std::string output1;
  //  const std::string input2;
  //  const std::string output2;
};

class HasherTests : public ::testing::TestWithParam<HasherTestParam>
{
public:
  HasherTests()
    : p{GetParam()}
  {}

  HasherTestParam p;
};

TEST_P(HasherTests, compatibility_with_Hash)
{
  auto hash = [this](byte_array_type const &s) {
    byte_array::ByteArray hash;

    switch (p.type)
    {
    case Hasher::MD5:
      hash = Hash<crypto::MD5>(s);
      break;
    case Hasher::SHA2_256:
      hash = Hash<crypto::SHA256>(s);
      break;
    case Hasher::SHA2_512:
      hash = Hash<crypto::SHA512>(s);
      break;
    case Hasher::SHA1:
      hash = Hash<crypto::SHA1>(s);
      break;
    case Hasher::FNV:
      hash = Hash<crypto::FNV>(s);
      break;
    }

    return byte_array::ToHex(hash);
  };  //???test hash sizes, consistency across calls

  EXPECT_EQ(hash(""), p.output_empty);
  //  EXPECT_EQ(hash(p.input1), p.output1);
  //  EXPECT_EQ(hash(p.input2), p.output2);

  //  byte_array_type input = "Hello world";
  //  EXPECT_EQ(hash(input), "64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c");
  //  input = "some RandSom byte_array!! With !@#$%^&*() Symbols!";
  //  EXPECT_EQ(hash(input), "3d4e08bae43f19e146065b7de2027f9a611035ae138a4ac1978f03cf43b61029");
}

std::vector<HasherTestParam> params{HasherTestParam(Hasher::FNV, ""),
                                    HasherTestParam(Hasher::MD5, "")};

INSTANTIATE_TEST_CASE_P(aaa, HasherTests, ::testing::ValuesIn(params));

//???
// void test_basic_hash(byte_array::ConstByteArray const &data_to_hash,
//                     byte_array::ConstByteArray const &expected_hash)
//{
//  FNV x;
//  x.Reset();
//  bool const retval = x.Update(data_to_hash);
//  ASSERT_TRUE(retval);
//  byte_array::ConstByteArray const hash = x.Final();
//
//  EXPECT_EQ(expected_hash, hash);
//}
//
// void test_basic_hash_value(byte_array::ConstByteArray const &               data_to_hash,
//                           fetch::crypto::detail::FNV1a::number_type const &expected_hash)
//{
//  FNV x;
//  x.Reset();
//  bool const retval = x.Update(data_to_hash);
//  ASSERT_TRUE(retval);
//  byte_array::ConstByteArray const bytes = x.Final();
//  auto const hash = *reinterpret_cast<std::size_t const *const>(bytes.pointer());
//
//  EXPECT_EQ(expected_hash, hash);
//}
//
// TEST_F(FNVTest, test_basic)
//{
//  fetch::crypto::detail::FNV1a::number_type const expected_hash = 0x406e475017aa7737;
//  byte_array::ConstByteArray const                expected_hash_array(
//                     reinterpret_cast<byte_array::ConstByteArray::value_type const
//                     *>(&expected_hash), sizeof(expected_hash));
//  test_basic_hash("abcdefg", expected_hash_array);
//  test_basic_hash_value("abcdefg", expected_hash);
//}

}  // namespace
