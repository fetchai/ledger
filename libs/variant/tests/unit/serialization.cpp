//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "variant/variant.hpp"

#include "gtest/gtest.h"

#include <stdexcept>
#include <string>

using namespace fetch;
using serializers::MsgPackSerializer;
using variant::Variant;

TEST(VariantSerialization, ArraySerialisation1)
{
  std::vector<int32_t> x = {3, 6, 9, 12};

  MsgPackSerializer serializer;
  serializer << x;
  serializer.seek(0);

  Variant y;
  serializer >> y;
  EXPECT_EQ(y.size(), 4);
  EXPECT_EQ(y[0].As<int32_t>(), 3);
  EXPECT_EQ(y[1].As<int32_t>(), 6);
  EXPECT_EQ(y[2].As<int32_t>(), 9);
  EXPECT_EQ(y[3].As<int32_t>(), 12);
}

TEST(VariantSerialization, ArraySerialisation2)
{
  Variant x = Variant::Array(4);
  x[0]      = 3;
  x[1]      = 6;
  x[2]      = 9;
  x[3]      = 12;

  MsgPackSerializer serializer;
  serializer << x;
  serializer.seek(0);

  std::vector<int32_t> y;
  serializer >> y;
  EXPECT_EQ(y.size(), 4);
  EXPECT_EQ(y[0], 3);
  EXPECT_EQ(y[1], 6);
  EXPECT_EQ(y[2], 9);
  EXPECT_EQ(y[3], 12);
}

TEST(VariantSerialization, ArrayArraySerialisation)
{
  Variant x = Variant::Array(4);
  x[0]      = Variant::Array(1);
  x[0][0]   = 1;
  x[1]      = Variant::Array(2);
  x[1][0]   = 2;
  x[1][1]   = 3;
  x[2]      = Variant::Array(3);
  x[2][0]   = 4;
  x[2][1]   = 5;
  x[2][2]   = 6;
  x[3]      = Variant::Array(4);
  x[3][0]   = 7;
  x[3][1]   = 8;
  x[3][2]   = 9;
  x[3][3]   = 10;

  MsgPackSerializer serializer;
  serializer << x;
  serializer.seek(0);

  std::vector<std::vector<uint64_t>> y;
  serializer >> y;
  EXPECT_EQ(y.size(), 4);
  EXPECT_EQ(y[0].size(), 1);
  EXPECT_EQ(y[1].size(), 2);
  EXPECT_EQ(y[2].size(), 3);
  EXPECT_EQ(y[3].size(), 4);

  EXPECT_EQ(y[0][0], 1);
  EXPECT_EQ(y[1][0], 2);
  EXPECT_EQ(y[1][1], 3);
  EXPECT_EQ(y[2][0], 4);
  EXPECT_EQ(y[2][1], 5);
  EXPECT_EQ(y[2][2], 6);
  EXPECT_EQ(y[3][0], 7);
  EXPECT_EQ(y[3][1], 8);
  EXPECT_EQ(y[3][2], 9);
  EXPECT_EQ(y[3][3], 10);
}

TEST(VariantSerialization, VariantInt)
{
  MsgPackSerializer stream;
  Variant           i1, i2;

  i1 = 123456;
  stream << i1;
  stream.seek(0);
  stream >> i2;
  EXPECT_EQ(i2.As<int>(), 123456);
}

TEST(VariantSerialization, VariantFloat)
{
  MsgPackSerializer stream;
  Variant           fp1, fp2;

  fp1 = 1.25;  // should be repn exactly.
  stream << fp1;
  stream.seek(0);
  stream >> fp2;
  EXPECT_EQ(fp2.As<float>(), 1.25);
}

TEST(VariantSerialization, VariantString)
{
  MsgPackSerializer stream;
  Variant           s1, s2;

  s1 = "123456";
  stream << s1;
  stream.seek(0);
  stream >> s2;
  EXPECT_EQ(s2.As<std::string>(), "123456");
}

TEST(VariantSerialization, VariantNull)
{
  MsgPackSerializer stream;
  Variant           null1, null2;

  null1 = Variant::Null();
  stream << null1;
  stream.seek(0);
  stream >> null2;
  EXPECT_EQ(null2.IsNull(), true);
}

TEST(VariantSerialization, DISABLED_VariantArray)
{
  MsgPackSerializer stream;
  Variant           arr1, arr2;

  arr1    = Variant::Array(4);
  arr1[0] = Variant::Null();
  arr1[1] = 123456;
  arr1[2] = 1.25;
  arr1[3] = "123456";
  stream << arr1;
  stream.seek(0);
  stream >> arr2;
  EXPECT_EQ(arr2.IsArray(), true);
  EXPECT_EQ(arr2[0].IsNull(), true);
  EXPECT_EQ(arr2[1].As<int>(), 123456);
  EXPECT_EQ(arr2[2].As<float>(), 1.25);
  EXPECT_EQ(arr2[3].As<std::string>(), "123456");
}

TEST(VariantSerialization, DISABLED_VariantArrayOfArray)
{
  MsgPackSerializer stream;
  Variant           arr1, arr2;

  arr1    = Variant::Array(4);
  arr1[0] = Variant::Array(4);
  arr1[1] = Variant::Array(4);
  arr1[2] = Variant::Array(4);
  arr1[3] = Variant::Array(4);

  for (std::size_t i = 0; i < 4; i++)
  {
    for (std::size_t j = 0; j < 4; j++)
    {
      arr1[i][j] = (i == j) ? 1 : 0;
    }
  }

  stream << arr1;
  stream.seek(0);
  stream >> arr2;

  EXPECT_EQ(arr2.IsArray(), true);
  for (std::size_t i = 0; i < 4; i++)
  {
    EXPECT_EQ(arr2[i].IsArray(), true);
    for (std::size_t j = 0; j < 4; j++)
    {
      EXPECT_EQ(arr2[i][j].As<int>(), (i == j) ? 1 : 0);
    }
  }
}

TEST(VariantSerialization, DISABLED_VariantObject)
{
  MsgPackSerializer stream;
  Variant           obj1, obj2;

  obj1 = Variant::Object();

  obj1["foo"] = 1;
  obj1["bar"] = 2;

  stream << obj1;
  stream.seek(0);
  stream >> obj2;

  EXPECT_EQ(obj2.IsObject(), true);
  EXPECT_EQ(obj2["foo"].As<int>(), 1);
  EXPECT_EQ(obj2["bar"].As<int>(), 2);
}
