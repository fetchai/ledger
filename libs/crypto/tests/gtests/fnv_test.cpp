//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "crypto/fnv.hpp"

#include "core/byte_array/encoders.hpp"
#include "gtest/gtest.h"
#include <iostream>
//#include "gmock/gmock.h"

namespace fetch {
namespace crypto {

namespace {

class FVNTest : public testing::Test
{
protected:
  void SetUp()
  {}

  void TearDown()
  {}

  void test_basic_hash(byte_array::ConstByteArray const &data_to_hash,
                       byte_array::ConstByteArray const &expected_hash)
  {
    FNV x;
    x.Reset();
    bool const retval = x.Update(data_to_hash);
    ASSERT_TRUE(retval);
    byte_array::ConstByteArray const hash = x.Final();

    EXPECT_EQ(expected_hash, hash);
  }

  void test_basic_hash_value(byte_array::ConstByteArray const &data_to_hash,
                             FNV::context_type const &         expected_hash)
  {
    FNV x;
    x.Reset();
    bool const retval = x.Update(data_to_hash);
    ASSERT_TRUE(retval);
    auto hash = x.Final<>();

    EXPECT_EQ(expected_hash, hash);
  }

  template<typename T, detail::eFnvAlgorithm ALGORITHM, std::size_t SIZE_IN_BYTES>
  void testFnvHash(
      detail::FNV<T, ALGORITHM, SIZE_IN_BYTES>& fnv,
      byte_array::ConstByteArray const &data_to_hash,
      T const &expected_hash)
  {
    fnv.reset();
    fnv.update(data_to_hash.pointer(), data_to_hash.size());
    T const resulting_hash = fnv.context();
    EXPECT_EQ(expected_hash, resulting_hash);
  }

  byte_array::ConstByteArray const data_to_hash {"asdfghjkl"};
};

TEST_F(FVNTest, test_basic)
{
  FNV::context_type const          expected_hash = 0x406e475017aa7737;
  byte_array::ConstByteArray const expected_hash_array(
      reinterpret_cast<byte_array::ConstByteArray::container_type const *>(&expected_hash),
      sizeof(expected_hash));
  test_basic_hash("abcdefg", expected_hash_array);
  test_basic_hash_value("abcdefg", expected_hash);
}

TEST_F(FVNTest, test_default_FNV_uses_std_size_t_and_fnv1a)
{
  using default_fnv = detail::FNV<>;
  EXPECT_EQ(detail::eFnvAlgorithm::fnv1a, default_fnv::algorithm);
  EXPECT_EQ(typeid(std::size_t), typeid(default_fnv::number_type));
}

TEST_F(FVNTest, test_FNV0_32bit)
{
  detail::FNV<uint32_t, detail::eFnvAlgorithm::fnv0_deprecated> fnv;
  testFnvHash(fnv, data_to_hash, 0xf78f889au);
}

TEST_F(FVNTest, test_FNV1_32bit)
{
  detail::FNV<uint32_t, detail::eFnvAlgorithm::fnv1> fnv;
  testFnvHash(fnv, data_to_hash, 0xc92ce8a9u);
}

TEST_F(FVNTest, test_FNV1a_32bit)
{
  detail::FNV<uint32_t, detail::eFnvAlgorithm::fnv1a> fnv;
  testFnvHash(fnv, data_to_hash, 0x2781041u);
}

TEST_F(FVNTest, test_FNV0_64bit)
{
  detail::FNV<uint64_t, detail::eFnvAlgorithm::fnv0_deprecated> fnv;
  testFnvHash(fnv, data_to_hash, 0xfef2bfb7764f7b1au);
}

TEST_F(FVNTest, test_FNV1_64bit)
{
  detail::FNV<uint64_t, detail::eFnvAlgorithm::fnv1> fnv;
  testFnvHash(fnv, data_to_hash, 0xc9cf9eecfdbf6de9u);
}

TEST_F(FVNTest, test_FNV1a_64bit)
{
  detail::FNV<uint64_t, detail::eFnvAlgorithm::fnv1a> fnv;
  testFnvHash(fnv, data_to_hash, 0xd16864d71e708e01u);
}

}  // namespace

}  // namespace crypto
}  // namespace fetch
