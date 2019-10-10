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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/main_serializer.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"

#include "gtest/gtest.h"

#include <stdexcept>

namespace {

using fetch::serializers::MsgPackSerializer;
using fetch::vm::Variant;
using fetch::vm::Object;
using fetch::vm::String;
using fetch::vm::Ptr;
using fetch::vm::Primitive;
using fetch::vm::TypeId;
using fetch::fixed_point::fp32_t;
using fetch::fixed_point::fp64_t;
using namespace fetch::vm::TypeIds;

class VariantSerialization : public ::testing::Test
{
public:
  bool     bl   = true;
  int8_t   i8   = -4;
  uint8_t  ui8  = 4;
  int16_t  i16  = -8;
  uint16_t ui16 = 8;
  int32_t  i32  = -16;
  uint32_t ui32 = 16;
  int64_t  i64  = -32;
  uint64_t ui64 = 32;
  float    ft32 = 64.321684f;
  double   ft64 = -128.64321684;
  fp32_t   fp32 = fetch::fixed_point::fp32_t::FromBase(i32);
  fp64_t   fp64 = fetch::fixed_point::fp64_t::FromBase(i64);

  std::string str{"I am a string"};
};

TEST_F(VariantSerialization, bool_variant)
{
  Primitive primitive;
  primitive.Set(bl);

  Variant           variant_in{primitive, Bool};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<bool>(), bl);
}

TEST_F(VariantSerialization, i8_variant)
{
  Primitive primitive;
  primitive.Set(i8);

  Variant           variant_in{primitive, Int8};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<int8_t>(), i8);
}

TEST_F(VariantSerialization, ui8_variant)
{
  Primitive primitive;
  primitive.Set(ui8);

  Variant           variant_in{primitive, UInt8};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<uint8_t>(), ui8);
}

TEST_F(VariantSerialization, i16_variant)
{
  Primitive primitive;
  primitive.Set(i16);

  Variant           variant_in{primitive, Int16};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<int16_t>(), i16);
}

TEST_F(VariantSerialization, ui16_variant)
{
  Primitive primitive;
  primitive.Set(ui16);

  Variant           variant_in{primitive, UInt16};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<uint16_t>(), ui16);
}

TEST_F(VariantSerialization, i32_variant)
{
  Primitive primitive;
  primitive.Set(i32);

  Variant           variant_in{primitive, Int32};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<int32_t>(), i32);
}

TEST_F(VariantSerialization, ui32_variant)
{
  Primitive primitive;
  primitive.Set(ui32);

  Variant           variant_in{primitive, UInt32};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<uint32_t>(), ui32);
}

TEST_F(VariantSerialization, i64_variant)
{
  Primitive primitive;
  primitive.Set(i64);

  Variant           variant_in{primitive, Int64};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<int64_t>(), i64);
}

TEST_F(VariantSerialization, ui64_variant)
{
  Primitive primitive;
  primitive.Set(ui64);

  Variant           variant_in{primitive, UInt64};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<uint64_t>(), ui64);
}

TEST_F(VariantSerialization, ft32_variant)
{
  Primitive primitive;
  primitive.Set(ft32);

  Variant           variant_in{primitive, Float32};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<float>(), ft32);
}

TEST_F(VariantSerialization, ft64_variant)
{
  Primitive primitive;
  primitive.Set(ft64);

  Variant           variant_in{primitive, Float64};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<double>(), ft64);
}

TEST_F(VariantSerialization, fp32_variant)
{
  Primitive primitive;
  primitive.Set(fp32);

  Variant           variant_in{primitive, Fixed32};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<fp32_t>(), fp32);
}

TEST_F(VariantSerialization, fp64_variant)
{
  Primitive primitive;
  primitive.Set(fp64);

  Variant           variant_in{primitive, Fixed64};
  MsgPackSerializer serializer{};
  serializer << variant_in;

  Variant           variant_out;
  MsgPackSerializer deserializer{serializer.data()};
  deserializer >> variant_out;

  ASSERT_EQ(variant_out.Get<fp64_t>(), fp64);
}

TEST_F(VariantSerialization, object_variant)
{
  Ptr<Object> object{new String(nullptr, std::string{"I am a string"})};

  Variant           variant_in{object, object->GetTypeId()};
  MsgPackSerializer serializer;
  ASSERT_THROW(serializer << variant_in, std::runtime_error);
}

}  // namespace
