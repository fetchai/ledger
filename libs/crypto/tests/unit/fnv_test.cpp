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
#include "crypto/fnv_detail.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <typeinfo>

namespace fetch {
namespace crypto {

namespace {

class FNVTest : public testing::Test
{
protected:
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
                             std::size_t const &               expected_hash)
  {
    FNV x;
    x.Reset();
    bool const retval = x.Update(data_to_hash);
    ASSERT_TRUE(retval);
    byte_array::ConstByteArray const bytes = x.Final();
    auto const hash = *reinterpret_cast<std::size_t const *const>(bytes.pointer());

    EXPECT_EQ(expected_hash, hash);
  }

  template <typename FNV_CONFIG, detail::eFnvAlgorithm ALGORITHM>
  void testFnvHash(detail::FNV<FNV_CONFIG, ALGORITHM> &    fnv,
                   byte_array::ConstByteArray const &      data_to_hash,
                   typename FNV_CONFIG::number_type const &expected_hash)
  {
    fnv.reset();
    fnv.update(data_to_hash.pointer(), data_to_hash.size());
    typename FNV_CONFIG::number_type const resulting_hash = fnv.context();
    EXPECT_EQ(expected_hash, resulting_hash);
  }

  byte_array::ConstByteArray const data_to_hash_{"asdfghjkl"};
};

TEST_F(FNVTest, test_basic)
{
  std::size_t const                expected_hash = 0x406e475017aa7737;
  byte_array::ConstByteArray const expected_hash_array(
      reinterpret_cast<byte_array::ConstByteArray::value_type const *>(&expected_hash),
      sizeof(expected_hash));
  test_basic_hash("abcdefg", expected_hash_array);
  test_basic_hash_value("abcdefg", expected_hash);
}

TEST_F(FNVTest, test_default_FNV_uses_std_size_t_and_fnv1a)
{
  using default_fnv = detail::FNV<>;
  EXPECT_EQ(detail::eFnvAlgorithm::fnv1a, default_fnv::algorithm);
  EXPECT_EQ(typeid(std::size_t), typeid(default_fnv::number_type));
}

TEST_F(FNVTest, test_FNV0_32bit)
{
  detail::FNV<detail::FNVConfig<uint32_t>, detail::eFnvAlgorithm::fnv0_deprecated> fnv;
  testFnvHash(fnv, data_to_hash_, uint32_t(0xf78f889au));
}

TEST_F(FNVTest, test_FNV1_32bit)
{
  detail::FNV<detail::FNVConfig<uint32_t>, detail::eFnvAlgorithm::fnv1> fnv;
  testFnvHash(fnv, data_to_hash_, uint32_t(0xc92ce8a9u));
}

TEST_F(FNVTest, test_FNV1a_32bit)
{
  detail::FNV<detail::FNVConfig<uint32_t>, detail::eFnvAlgorithm::fnv1a> fnv;
  testFnvHash(fnv, data_to_hash_, uint32_t(0x2781041u));
}

TEST_F(FNVTest, test_FNV0_64bit)
{
  detail::FNV<detail::FNVConfig<uint64_t>, detail::eFnvAlgorithm::fnv0_deprecated> fnv;
  testFnvHash(fnv, data_to_hash_, uint64_t(0xfef2bfb7764f7b1au));
}

TEST_F(FNVTest, test_FNV1_64bit)
{
  detail::FNV<detail::FNVConfig<uint64_t>, detail::eFnvAlgorithm::fnv1> fnv;
  testFnvHash(fnv, data_to_hash_, uint64_t(0xc9cf9eecfdbf6de9u));
}

TEST_F(FNVTest, test_FNV1a_64bit)
{
  detail::FNV<detail::FNVConfig<uint64_t>, detail::eFnvAlgorithm::fnv1a> fnv;
  testFnvHash(fnv, data_to_hash_, uint64_t(0xd16864d71e708e01u));
}

}  // namespace
}  // namespace crypto
}  // namespace fetch
