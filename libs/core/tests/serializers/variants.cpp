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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"

#include "variant/variant.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

TEST(MsgPacker, VariantInt)
{

  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               i1, i2;

    i1     = 123456;
    stream = fetch::serializers::MsgPackSerializer();
    stream << i1;
    stream.seek(0);
    stream >> i2;
    EXPECT_EQ(i2.As<int>(), 123456);
  }
}
TEST(MsgPacker, VariantFloat)
{
  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               fp1, fp2;

    fp1    = 1.25;  // should be repn exactly.
    stream = fetch::serializers::MsgPackSerializer();
    stream << fp1;
    stream.seek(0);
    stream >> fp2;
    EXPECT_EQ(fp2.As<float>(), 1.25);
  }
}
TEST(MsgPacker, VariantString)
{
  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               s1, s2;

    s1     = "123456";
    stream = fetch::serializers::MsgPackSerializer();
    stream << s1;
    stream.seek(0);
    stream >> s2;
    EXPECT_EQ(s2.As<std::string>(), "123456");
  }
}
TEST(MsgPacker, VariantNull)
{
  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               null1, null2;

    null1  = fetch::variant::Variant::Null();
    stream = fetch::serializers::MsgPackSerializer();
    stream << null1;
    stream.seek(0);
    stream >> null2;
    EXPECT_EQ(null2.IsNull(), true);
  }
}
TEST(MsgPacker, VariantArray)
{
  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               arr1, arr2;

    arr1    = fetch::variant::Variant::Array(4);
    arr1[0] = fetch::variant::Variant::Null();
    arr1[1] = 123456;
    arr1[2] = 1.25;
    arr1[3] = "123456";
    stream  = fetch::serializers::MsgPackSerializer();
    stream << arr1;
    stream.seek(0);
    stream >> arr2;
    EXPECT_EQ(arr2.IsArray(), true);
    EXPECT_EQ(arr2[0].IsNull(), true);
    EXPECT_EQ(arr2[1].As<int>(), 123456);
    EXPECT_EQ(arr2[2].As<float>(), 1.25);
    EXPECT_EQ(arr2[3].As<std::string>(), "123456");
  }
}
TEST(MsgPacker, VariantArrayOfArray)
{

  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               arr1, arr2;

    arr1    = fetch::variant::Variant::Array(4);
    arr1[0] = fetch::variant::Variant::Array(4);
    arr1[1] = fetch::variant::Variant::Array(4);
    arr1[2] = fetch::variant::Variant::Array(4);
    arr1[3] = fetch::variant::Variant::Array(4);

    for (std::size_t i = 0; i < 4; i++)
    {
      for (std::size_t j = 0; j < 4; j++)
      {
        arr1[i][j] = (i == j) ? 1 : 0;
      }
    }

    stream = fetch::serializers::MsgPackSerializer();
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
}
TEST(MsgPacker, VariantObject)
{
  {
    fetch::serializers::MsgPackSerializer stream;
    fetch::variant::Variant               obj1, obj2;

    obj1 = fetch::variant::Variant::Object();

    obj1["foo"] = 1;
    obj1["bar"] = 2;

    stream = fetch::serializers::MsgPackSerializer();
    stream << obj1;
    stream.seek(0);
    stream >> obj2;

    EXPECT_EQ(obj2.IsObject(), true);
    EXPECT_EQ(obj2["foo"].As<int>(), 1);
    EXPECT_EQ(obj2["bar"].As<int>(), 2);
  }
}
