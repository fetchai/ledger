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
#include "core/serialisers/main_serialiser.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/variant.hpp"

#include "gtest/gtest.h"

#include <stdexcept>

namespace {

using fetch::serialisers::MsgPackSerialiser;
using fetch::vm::Variant;
using fetch::vm::Object;
using fetch::vm::String;
using fetch::vm::Ptr;
using fetch::vm::Primitive;
using fetch::vm::TypeId;
using fetch::fixed_point::fp32_t;
using fetch::fixed_point::fp64_t;
using fetch::fixed_point::fp128_t;
using namespace fetch::vm::TypeIds;

class VariantSerialization : public ::testing::Test
{
public:
  MsgPackSerialiser serialiser;
  MsgPackSerialiser deserialiser;
  Variant           variant_in, variant_out;
  Primitive         primitive;
  Ptr<Object>       object;

  bool     bl    = true;
  int8_t   i8    = -4;
  uint8_t  ui8   = 4;
  int16_t  i16   = -8;
  uint16_t ui16  = 8;
  int32_t  i32   = -16;
  uint32_t ui32  = 16;
  int64_t  i64   = -32;
  uint64_t ui64  = 32;
  fp32_t   fp32  = fetch::fixed_point::fp32_t::FromBase(i32);
  fp64_t   fp64  = fetch::fixed_point::fp64_t::FromBase(i64);
  fp128_t  fp128 = fetch::fixed_point::fp128_t::FromBase(i64);

  std::string str{"I am a string"};

  void SerialiseAs(TypeId type)
  {
    variant_in = Variant(primitive, type);
    serialiser << variant_in;
    deserialiser = MsgPackSerialiser{serialiser.data()};
    deserialiser >> variant_out;
  }

  bool SerialiseAsString()
  {
    object     = Ptr<Object>{new String(nullptr, std::string{"I am a string"})};
    variant_in = Variant(object, object->GetTypeId());
    serialiser << variant_in;
    deserialiser = MsgPackSerialiser(serialiser.data());
    deserialiser >> variant_out;
    return variant_out.Get<Ptr<String>>()->string() == str;
  }
};

TEST_F(VariantSerialization, bool_variant)
{
  primitive.Set(bl);
  SerialiseAs(Bool);
  ASSERT_EQ(variant_out.Get<bool>(), bl);
}

TEST_F(VariantSerialization, i8_variant)
{
  primitive.Set(i8);
  SerialiseAs(Int8);
  ASSERT_EQ(variant_out.Get<int8_t>(), i8);
}

TEST_F(VariantSerialization, ui8_variant)
{
  primitive.Set(ui8);
  SerialiseAs(UInt8);
  ASSERT_EQ(variant_out.Get<uint8_t>(), ui8);
}

TEST_F(VariantSerialization, i16_variant)
{
  primitive.Set(i16);
  SerialiseAs(Int16);
  ASSERT_EQ(variant_out.Get<int16_t>(), i16);
}

TEST_F(VariantSerialization, ui16_variant)
{
  primitive.Set(ui16);
  SerialiseAs(UInt16);
  ASSERT_EQ(variant_out.Get<uint16_t>(), ui16);
}

TEST_F(VariantSerialization, i32_variant)
{
  primitive.Set(i32);
  SerialiseAs(Int32);
  ASSERT_EQ(variant_out.Get<int32_t>(), i32);
}

TEST_F(VariantSerialization, ui32_variant)
{
  primitive.Set(ui32);
  SerialiseAs(UInt32);
  ASSERT_EQ(variant_out.Get<uint32_t>(), ui32);
}

TEST_F(VariantSerialization, i64_variant)
{
  primitive.Set(i64);
  SerialiseAs(Int64);
  ASSERT_EQ(variant_out.Get<int64_t>(), i64);
}

TEST_F(VariantSerialization, ui64_variant)
{
  primitive.Set(ui64);
  SerialiseAs(UInt64);
  ASSERT_EQ(variant_out.Get<uint64_t>(), ui64);
}

TEST_F(VariantSerialization, fp32_variant)
{
  primitive.Set(fp32);
  SerialiseAs(Fixed32);
  ASSERT_EQ(variant_out.Get<fp32_t>(), fp32);
}

TEST_F(VariantSerialization, fp64_variant)
{
  primitive.Set(fp64);
  SerialiseAs(Fixed64);
  ASSERT_EQ(variant_out.Get<fp64_t>(), fp64);
}

TEST_F(VariantSerialization, object_variant)
{
  ASSERT_THROW(SerialiseAsString(), std::runtime_error);
}

}  // namespace
