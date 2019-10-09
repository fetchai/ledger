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
using namespace fetch::vm::TypeIds;

class VariantSerialization : public ::testing::Test
{
public:
  MsgPackSerializer serializer;
  MsgPackSerializer deserializer;
  Variant           variant_in, variant_out;
  Primitive         primitive;
  Ptr<Object>       object;

  bool     bl   = true;
  int8_t   i8   = -4;
  uint8_t  ui8  = 4;
  int16_t  i16  = -8;
  uint16_t ui16 = 8;
  int32_t  i32  = -16;
  uint32_t ui32 = 16;
  int64_t  i64  = -32;
  uint64_t ui64 = 32;
  float    f32  = 64.321684f;
  double   f64  = -128.64321684;

  std::string str{"I am a string"};

  void Reset()
  {
    serializer   = MsgPackSerializer{};
    deserializer = MsgPackSerializer{};
    variant_in   = Variant{};
    variant_out  = Variant{};
    primitive    = Primitive{};
    object       = Ptr<Object>{};
  }

  void SerializeAs(TypeId type)
  {
    variant_in = Variant(primitive, type);
    serializer << variant_in;
    deserializer = MsgPackSerializer{serializer.data()};
    deserializer >> variant_out;
  }

  bool SerializeAsString()
  {
    object     = Ptr<Object>{new String(nullptr, std::string{"I am a string"})};
    variant_in = Variant(object, object->GetTypeId());
    serializer << variant_in;
    deserializer = MsgPackSerializer(serializer.data());
    deserializer >> variant_out;
    return variant_out.Get<Ptr<String>>()->str == str;
  }
};

TEST_F(VariantSerialization, primitive_types_serialization)
{
  primitive.Set(bl);
  SerializeAs(Bool);
  ASSERT_EQ(variant_out.Get<bool>(), bl);

  Reset();
  primitive.Set(i8);
  SerializeAs(Int8);
  ASSERT_EQ(variant_out.Get<int8_t>(), i8);

  Reset();
  primitive.Set(ui8);
  SerializeAs(UInt8);
  ASSERT_EQ(variant_out.Get<uint8_t>(), ui8);

  Reset();
  primitive.Set(i16);
  SerializeAs(Int16);
  ASSERT_EQ(variant_out.Get<int16_t>(), i16);

  Reset();
  primitive.Set(ui16);
  SerializeAs(UInt16);
  ASSERT_EQ(variant_out.Get<uint16_t>(), ui16);

  Reset();
  primitive.Set(i32);
  SerializeAs(Int32);
  ASSERT_EQ(variant_out.Get<int32_t>(), i32);

  Reset();
  primitive.Set(ui32);
  SerializeAs(UInt32);
  ASSERT_EQ(variant_out.Get<uint32_t>(), ui32);

  Reset();
  primitive.Set(i64);
  SerializeAs(Int64);
  ASSERT_EQ(variant_out.Get<int64_t>(), i64);

  Reset();
  primitive.Set(ui64);
  SerializeAs(UInt64);
  ASSERT_EQ(variant_out.Get<uint64_t>(), ui64);

  Reset();
  primitive.Set(f32);
  SerializeAs(Float32);
  ASSERT_EQ(variant_out.Get<float>(), f32);

  Reset();
  primitive.Set(f32);
  SerializeAs(Fixed32);
  ASSERT_EQ(variant_out.Get<float>(), f32);

  Reset();
  primitive.Set(f64);
  SerializeAs(Float64);
  ASSERT_EQ(variant_out.Get<double>(), f64);

  Reset();
  primitive.Set(f64);
  SerializeAs(Fixed64);
  ASSERT_EQ(variant_out.Get<double>(), f64);
}

TEST_F(VariantSerialization, object_types_serialization)
{
  Reset();
  ASSERT_THROW(SerializeAsString(), std::runtime_error);
}

}  // namespace
