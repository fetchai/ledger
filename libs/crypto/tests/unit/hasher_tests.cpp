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
#include <ostream>
#include <utility>

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
  HasherTestParam(Hasher type_, byte_array::ByteArray output_empty_, byte_array::ByteArray output1_,
                  byte_array::ByteArray output2_)
    : type{type_}
    , output_empty{std::move(output_empty_)}
    , input1{"Hello world"}
    , output1{std::move(output1_)}
    , input2{"some ArbitrSary byte_array!! With !@#$%^&*() Symbols!"}
    , output2{std::move(output2_)}
  {}

  Hasher                type;
  byte_array::ByteArray output_empty;
  const std::string     input1;
  const std::string     output1;
  const std::string     input2;
  const std::string     output2;
};

std::ostream &operator<<(std::ostream &os, HasherTestParam const &p)
{
  switch (p.type)
  {
  case Hasher::MD5:
    os << "MD5";
    break;
  case Hasher::SHA2_256:
    os << "SHA2_256";
    break;
  case Hasher::SHA2_512:
    os << "SHA2_512";
    break;
  case Hasher::SHA1:
    os << "SHA1";
    break;
  case Hasher::FNV:
    os << "FNV";
    break;
  }

  os << "??? ???" << std::endl;

  return os;
}

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
  EXPECT_EQ(hash(p.input1), p.output1);
  EXPECT_EQ(hash(p.input2), p.output2);

  //  byte_array_type input = "Hello world";
  //  EXPECT_EQ(hash(input), "64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c");
  //  input = "some RandSom byte_array!! With !@#$%^&*() Symbols!";
  //  EXPECT_EQ(hash(input), "3d4e08bae43f19e146065b7de2027f9a611035ae138a4ac1978f03cf43b61029");
}

std::vector<HasherTestParam> params{
    HasherTestParam(Hasher::FNV, "25232284e49cf2cb", "c76437a385f71327", "5e09a4e759bf7dc0"),
    HasherTestParam(Hasher::MD5, "d41d8cd98f00b204e9800998ecf8427e",
                    "3e25960a79dbc69b674cd4ec67a72c62", "47c25e9489ad6cab8ca1dc29cd90ac74"),
    HasherTestParam(Hasher::SHA2_256,
                    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                    "64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c",
                    "c538cb52521023c3e430d58eedd3630ae2e12b5f9a027129f1da023a2c093360"),
    HasherTestParam(Hasher::SHA2_512,
                    "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2"
                    "b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
                    "b7f783baed8297f0db917462184ff4f08e69c2d5e5f79a942600f9725f58ce1f29c18139bf80b0"
                    "6c0fff2bdd34738452ecf40c488c22a7e3d80cdf6f9c1c0d47",
                    "8510b88fcd1bb053aa7dac591ec42e7c61557649750139d84fea805b8a8d69f8790235c49a8168"
                    "f8e2b3bfcfb03be4e1007d612d4fbfebbaa8d51e44cd5431ad"),
    HasherTestParam(Hasher::SHA1, "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                    "7b502c3a1f48c8609ae212cdfb639dee39673f5e",
                    "9654b13e864968ab29cf2cf10654e826ed2a57d9")};

INSTANTIATE_TEST_CASE_P(aaa, HasherTests, ::testing::ValuesIn(params), );

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
