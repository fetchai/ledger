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
#include <iostream>
#include "gtest/gtest.h"
//#include "gmock/gmock.h"

namespace fetch {
namespace crypto {

namespace {

class FVNTest : public testing::Test
{
protected:

  void SetUp() {}

  void TearDown() {}

  void test_basic_hash(byte_array::ConstByteArray const& data_to_hash, byte_array::ConstByteArray const& expected_hash)
  {
    FNV x;
    x.Reset();
    bool const retval = x.Update(data_to_hash);
    ASSERT_TRUE(retval);
    byte_array::ConstByteArray const hash = x.Final();

    EXPECT_EQ(expected_hash, hash);
  }

  void test_basic_hash_value(byte_array::ConstByteArray const& data_to_hash, FNV::context_type const& expected_hash)
  {
    FNV x;
    x.Reset();
    bool const retval = x.Update(data_to_hash);
    ASSERT_TRUE(retval);
    auto hash = x.Final<FNV::context_type>();

    EXPECT_EQ(expected_hash, hash);
  }
};

TEST_F(FVNTest, test_basic)
{
    FNV::context_type const expected_hash = 0x406e475017aa7737;
    byte_array::ConstByteArray const expected_hash_array (reinterpret_cast<byte_array::ConstByteArray::container_type const*>(&expected_hash),
                                                          sizeof(expected_hash));
    test_basic_hash("abcdefg", expected_hash_array);
    test_basic_hash_value("abcdefg", expected_hash);
}

}  // namespace

}  // namespace crypto
}  // namespace fetch
