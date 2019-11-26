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
#include "vectorise/uint/uint.hpp"
#include "vm/module.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/math/bignumber.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace math {
namespace {
Ptr<String> ToString(VM *vm, Ptr<UInt256Wrapper> const &n)
{
  return Ptr<String>{new String{vm, static_cast<std::string>(n->number())}};
}

template <typename T>
meta::IfIsInteger<T, T> ToInteger(VM * /*vm*/, Ptr<UInt256Wrapper> const &a)
{
  return {*reinterpret_cast<T const *>(a->number().pointer())};
}

Ptr<UInt256Wrapper> ConstructorFromBytesBigEndian(VM *vm, TypeId type_id,
                                                  Ptr<ByteArrayWrapper> const &ba)
{
  try
  {
    return Ptr<UInt256Wrapper>{
        new UInt256Wrapper(vm, type_id, ba->byte_array(), memory::Endian::BIG)};
  }
  catch (std::exception const &e)
  {
    vm->RuntimeError(e.what());
  }
  return {};
}

}  // namespace

void UInt256Wrapper::Bind(Module &module)
{
  module.CreateClassType<UInt256Wrapper>("UInt256")
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) { return UInt256Wrapper::Constructor(vm, type_id, 0u); })
      .CreateConstructor(&UInt256Wrapper::Constructor)
      .CreateConstructor(&ConstructorFromBytesBigEndian)
      .EnableOperator(Operator::Equal)
      .EnableOperator(Operator::NotEqual)
      .EnableOperator(Operator::LessThan)
      .EnableOperator(Operator::LessThanOrEqual)
      .EnableOperator(Operator::GreaterThan)
      .EnableOperator(Operator::GreaterThanOrEqual)
      //.CreateMemberFunction("toBuffer", &UInt256Wrapper::ToBuffer)
      .CreateMemberFunction("increase", &UInt256Wrapper::Increase)
      .CreateMemberFunction("logValue", &UInt256Wrapper::LogValue)
      .CreateMemberFunction("size", &UInt256Wrapper::size);

  module.CreateFreeFunction("toString", &ToString);
  module.CreateFreeFunction("toBuffer", [](VM *vm, Ptr<UInt256Wrapper> const &x) {
    return vm->CreateNewObject<ByteArrayWrapper>(
        x->number().As<byte_array::ByteArray>(memory::Endian::BIG, true));
  });
  module.CreateFreeFunction(
      "toFloat64", [](VM * /*vm*/, Ptr<UInt256Wrapper> const &x) { return ToDouble(x->number()); });
  module.CreateFreeFunction("toUInt64", &ToInteger<uint64_t>);
  module.CreateFreeFunction("toInt64", &ToInteger<int64_t>);
  module.CreateFreeFunction("toUInt32", &ToInteger<uint32_t>);
  module.CreateFreeFunction("toInt32", &ToInteger<int32_t>);
}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, UInt256 data)
  : Object(vm, type_id)
  , number_(std::move(data))
{}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, byte_array::ConstByteArray const &data,
                               memory::Endian endianess_of_input_data)
  : Object(vm, type_id)
  , number_(data, endianess_of_input_data)
{}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, uint64_t data)
  : Object(vm, type_id)
  , number_(data)
{}

Ptr<UInt256Wrapper> UInt256Wrapper::Constructor(VM *vm, TypeId type_id, uint64_t val)
{
  try
  {
    return Ptr<UInt256Wrapper>{new UInt256Wrapper(vm, type_id, val)};
  }
  catch (std::exception const &e)
  {
    vm->RuntimeError(e.what());
  }
  return {};
}

double UInt256Wrapper::LogValue() const
{
  return Log(number_);
}

void UInt256Wrapper::Increase()
{
  ++number_;
}

fetch::math::SizeType UInt256Wrapper::size() const
{
  return number_.size();
}

vectorise::UInt<256> const &UInt256Wrapper::number() const
{
  return number_;
}

bool UInt256Wrapper::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << number_;
  return true;
}

bool UInt256Wrapper::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> number_;
  return true;
}

bool UInt256Wrapper::ToJSON(JSONVariant &variant)
{
  variant = JSONVariant::Object();
  byte_array::ByteArray value(32);
  for (uint64_t i = 0; i < 32; ++i)
  {
    value[i] = number_[i];
  }

  variant["type"]  = GetTypeName();
  variant["value"] = ToHex(value);
  return true;
}

bool UInt256Wrapper::FromJSON(JSONVariant const &variant)
{
  if (!variant.IsObject())
  {
    vm_->RuntimeError("JSON deserialisation of " + GetTypeName() + " must be an object.");
    return false;
  }

  if (!variant.Has("type"))
  {
    vm_->RuntimeError("JSON deserialisation of " + GetTypeName() + " must have field 'type'.");
    return false;
  }

  if (!variant.Has("value"))
  {
    vm_->RuntimeError("JSON deserialisation of " + GetTypeName() + " must have field 'value'.");
    return false;
  }

  if (variant["type"].As<std::string>() != GetTypeName())
  {
    vm_->RuntimeError("Field 'type' must be '" + GetTypeName() + "'.");
    return false;
  }

  if (!variant["value"].IsString())
  {
    vm_->RuntimeError("Field 'value' must be a hex-encoded string.");
    return false;
  }

  byte_array::ByteArray value;
  try
  {
    value = FromHex(variant["value"].As<byte_array::ConstByteArray>());
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Field 'value' must be a hex-encoded string.");
    return false;
  }

  uint64_t i = 0;
  uint64_t n = std::min(32ul, value.size());
  for (; i < n; ++i)
  {
    number_[i] = value[i];
  }

  for (; i < 32; ++i)
  {
    number_[i] = 0;
  }

  return true;
}

bool UInt256Wrapper::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ == rhs->number_;
}

bool UInt256Wrapper::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ != rhs->number_;
}

bool UInt256Wrapper::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ < rhs->number_;
}

bool UInt256Wrapper::IsLessThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                       fetch::vm::Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return lhs->number_ <= rhs->number_;
}

bool UInt256Wrapper::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return rhs->number_ < lhs->number_;
}

bool UInt256Wrapper::IsGreaterThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                          fetch::vm::Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return rhs->number_ <= lhs->number_;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
