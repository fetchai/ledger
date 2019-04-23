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

#include "core/serializers/byte_array_buffer.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class SerializersTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<int, long, float, double>;
TYPED_TEST_CASE(SerializersTest, MyTypes);

TYPED_TEST(SerializersTest, serialize_empty_tensor)
{
  fetch::math::Tensor<TypeParam>      t1;
  fetch::serializers::ByteArrayBuffer b;
  b << t1;
  b.seek(0);
  fetch::math::Tensor<TypeParam> t2;
  b >> t2;
  EXPECT_EQ(t1, t2);
}

TYPED_TEST(SerializersTest, serialize_tensor)
{
  fetch::math::Tensor<TypeParam> t1({2, 3, 4, 5, 6});
  TypeParam                      i(0);
  for (auto &e : t1)
  {
    e = i;
    i++;
  }
  fetch::serializers::ByteArrayBuffer b;

  b << t1;
  b.seek(0);
  fetch::math::Tensor<TypeParam> t2;

  b >> t2;

  EXPECT_EQ(t1, t2);
}
