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
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include "gtest/gtest.h"

using namespace fetch;
using namespace fetch::crypto;

using byte_array_type = byte_array::ByteArray;

TEST(crypto_SHA_gtest, The_SHA256_implmentation_differs_from_other_libraries)
{
  auto hash = [](byte_array_type const &s) { return byte_array::ToHex(Hash<crypto::SHA256>(s)); };

  byte_array_type input = "Hello world";
  EXPECT_EQ(hash(input), "64ec88ca00b268e5ba1a35678a1b5316d212f4f366b2477232534a8aeca37f3c");
  input = "some RandSom byte_array!! With !@#$%^&*() Symbols!";
  EXPECT_EQ(hash(input), "3d4e08bae43f19e146065b7de2027f9a611035ae138a4ac1978f03cf43b61029");
}
