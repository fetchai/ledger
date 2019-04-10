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

#include "crypto/openssl_common.hpp"
#include "gtest/gtest.h"

namespace fetch {
namespace crypto {
namespace openssl {

namespace {

enum class ePadding : uint8_t
{
  prefix,
  suffix
};

class OpenSslBIGNUMPaddingTest : public testing::Test
{
protected:
  virtual void SetUp()
  {}

  virtual void TearDown()
  {}

  void test_convert_from_bin_to_BN_with_padding(byte_array::ConstByteArray const &orig_bin_bn,
                                                std::size_t const num_of_padding_bytes,
                                                ePadding const    padding,
                                                bool const        expected_comparison_result = true)
  {
    shrd_ptr_type<BIGNUM> orig_bn{BN_new()};

    ASSERT_NE(nullptr, BN_bin2bn(orig_bin_bn.pointer(), static_cast<int>(orig_bin_bn.size()),
                                 orig_bn.get()));

    byte_array::ConstByteArray const padding_bin(num_of_padding_bytes);
    byte_array::ConstByteArray       padded_bin_bn;

    switch (padding)
    {
    case ePadding::prefix:
      std::cerr << "Padbin size: " << padding_bin.size()
                << "; orig_bin size: " << orig_bin_bn.size() << '\n';
      padded_bin_bn = padding_bin + orig_bin_bn;
      break;
    case ePadding::suffix:
      padded_bin_bn = orig_bin_bn + padding_bin;
      break;
    };

    ASSERT_EQ(num_of_padding_bytes, padding_bin.size());
    ASSERT_EQ(orig_bin_bn.size() + padding_bin.size(), padded_bin_bn.size());

    for (std::size_t i = 0; i < padding_bin.size(); ++i)
    {
      ASSERT_EQ(0, padding_bin[i]);
    }

    switch (padding)
    {
    case ePadding::prefix:
      ASSERT_EQ(padding_bin, padded_bin_bn.SubArray(0, padding_bin.size()));
      break;
    case ePadding::suffix:
      ASSERT_EQ(padding_bin, padded_bin_bn.SubArray(padded_bin_bn.size() - padding_bin.size(),
                                                    padding_bin.size()));
      break;
    };

    shrd_ptr_type<BIGNUM> padded_bn{BN_new()};

    ASSERT_NE(nullptr,
              BN_bin2bn(static_cast<byte_array::ConstByteArray const &>(padded_bin_bn).pointer(),
                        static_cast<int>(padded_bin_bn.size()), padded_bn.get()));

    EXPECT_EQ(expected_comparison_result, 0 == BN_cmp(orig_bn.get(), padded_bn.get()));
  }
};

TEST_F(OpenSslBIGNUMPaddingTest, test_convert_from_bin_to_BN_with_prefix_padding)
{
  byte_array::ConstByteArray const x_bin({1, 2, 3, 4, 5});
  test_convert_from_bin_to_BN_with_padding(x_bin, 5, ePadding::prefix, true);
}

TEST_F(OpenSslBIGNUMPaddingTest,
       test_convert_from_bin_to_BN_with_suffix_padding_is_supposed_to_fail)
{
  byte_array::ConstByteArray const x_bin({1, 2, 3, 4, 5});
  test_convert_from_bin_to_BN_with_padding(x_bin, 5, ePadding::suffix, false);
}

}  // namespace

}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
