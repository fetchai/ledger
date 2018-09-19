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

#include "core/script/variant.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"
#include <gtest/gtest.h>

using namespace fetch;
using namespace fetch::script;
using namespace fetch::serializers;

TEST(variant_test, variant_basic_functionality)
{
  VariantArray arr = VariantArray(100);
  for (std::size_t i = 0; i < arr.size(); ++i)
  {
    arr[i] = i;
  }

  VariantArray other(arr, 50, 10);
  EXPECT_TRUE(other.size() == 10);

  for (std::size_t i = 0; i < other.size(); ++i)
  {
    EXPECT_TRUE(other[i].As<std::size_t>() == (50 + i));
  }
}
