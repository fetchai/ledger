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
#include <vector>

namespace {

using namespace fetch;
using namespace fetch::byte_array;
using namespace fetch::crypto;

enum class Hasher
{
  MD5,
  SHA2_256,
  SHA2_512,
  SHA1,
  FNV
};

std::size_t hash_size(Hasher hasher_type)
{
  std::size_t hash_size = 0u;

  switch (hasher_type)
  {
  case Hasher::MD5:
    hash_size = crypto::MD5::size_in_bytes;
    break;
  case Hasher::SHA2_256:
    hash_size = crypto::SHA256::size_in_bytes;
    break;
  case Hasher::SHA2_512:
    hash_size = crypto::SHA512::size_in_bytes;
    break;
  case Hasher::SHA1:
    hash_size = crypto::SHA1::size_in_bytes;
    break;
  case Hasher::FNV:
    hash_size = crypto::FNV::size_in_bytes;
    break;
  }

  return hash_size;
}

class HasherTestParam
{
public:
  HasherTestParam(Hasher type_, ByteArray output_empty_, ByteArray output1_, ByteArray output2_,
                  ByteArray output3_)
    : type{type_}
    , expected_size{hash_size(type)}
    , input_empty("")
    , expected_output_empty(std::move(output_empty_))
    , input1("Hello world")
    , expected_output1(std::move(output1_))
    , input2("abcdefg")
    , expected_output2(std::move(output2_))
    , input3("some ArbitrSary byte_array!! With !@#$%^&*() Symbols!")
    , expected_output3(std::move(output3_))
    , all_inputs{input_empty, input1, input2, input3}
  {}

  const Hasher                 type;
  const std::size_t            expected_size;
  const ByteArray              input_empty;
  const ByteArray              expected_output_empty;
  const ByteArray              input1;
  const ByteArray              expected_output1;
  const ByteArray              input2;
  const ByteArray              expected_output2;
  const ByteArray              input3;
  const ByteArray              expected_output3;
  const std::vector<ByteArray> all_inputs;
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

  return os;
}

class HasherTests : public ::testing::TestWithParam<HasherTestParam>
{
public:
  HasherTests()
    : p{GetParam()}
  {}

  ByteArray hash(ByteArray const &s)
  {
    ByteArray hash;

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

    return hash;
  };

  HasherTestParam p;
};

TEST_P(HasherTests, hash_is_consistent_across_calls)
{
  for (auto const &input : p.all_inputs)
  {
    auto const hash1 = hash(input);
    auto const hash2 = hash(input);
    auto const hash3 = hash(input);

    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash1, hash3);
    EXPECT_EQ(hash2, hash3);
  }
}

TEST_P(HasherTests, hash_size)
{
  for (auto const &input : p.all_inputs)
  {
    EXPECT_EQ(hash(input).size(), p.expected_size);
  }
}

TEST_P(HasherTests, empty_input)
{
  EXPECT_EQ(ToHex(hash(p.input_empty)), p.expected_output_empty);
}

TEST_P(HasherTests, non_empty_inputs)
{
  EXPECT_EQ(ToHex(hash(p.input1)), p.expected_output1);
  EXPECT_EQ(ToHex(hash(p.input2)), p.expected_output2);
  EXPECT_EQ(ToHex(hash(p.input3)), p.expected_output3);
}

// clang-format off
std::vector<HasherTestParam> params{
    HasherTestParam(Hasher::FNV,
                    "25232284e49cf2cb",
                    "c76437a385f71327",
                    "3777aa1750476e40",
                    "5e09a4e759bf7dc0"),
    HasherTestParam(Hasher::MD5,
                    "d41d8cd98f00b204e9800998ecf8427e",
                    "3e25960a79dbc69b674cd4ec67a72c62",
                    "7ac66c0f148de9519b8bd264312c4d64",
                    "47c25e9489ad6cab8ca1dc29cd90ac74"),
    HasherTestParam(Hasher::SHA2_256,
                    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                    "64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c",
                    "7d1a54127b222502f5b79b5fb0803061152a44f92b37e23c6527baf665d4da9a",
                    "c538cb52521023c3e430d58eedd3630ae2e12b5f9a027129f1da023a2c093360"),
    HasherTestParam(Hasher::SHA2_512,
                    "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
                    "b7f783baed8297f0db917462184ff4f08e69c2d5e5f79a942600f9725f58ce1f29c18139bf80b06c0fff2bdd34738452ecf40c488c22a7e3d80cdf6f9c1c0d47",
                    "d716a4188569b68ab1b6dfac178e570114cdf0ea3a1cc0e31486c3e41241bc6a76424e8c37ab26f096fc85ef9886c8cb634187f4fddff645fb099f1ff54c6b8c",
                    "8510b88fcd1bb053aa7dac591ec42e7c61557649750139d84fea805b8a8d69f8790235c49a8168f8e2b3bfcfb03be4e1007d612d4fbfebbaa8d51e44cd5431ad"),
    HasherTestParam(Hasher::SHA1,
                    "da39a3ee5e6b4b0d3255bfef95601890afd80709",
                    "7b502c3a1f48c8609ae212cdfb639dee39673f5e",
                    "2fb5e13419fc89246865e7a324f476ec624e8740",
                    "9654b13e864968ab29cf2cf10654e826ed2a57d9")};
// clang-format on

INSTANTIATE_TEST_CASE_P(HasherTestsPrefix, HasherTests, ::testing::ValuesIn(params), );

}  // namespace
