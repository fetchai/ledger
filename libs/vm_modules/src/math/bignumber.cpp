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

Ptr<String> UInt256Wrapper::ToString(VM *vm, Ptr<UInt256Wrapper> const &n)
{
  byte_array::ByteArray ba(32);
  for (uint64_t i = 0; i < 32; ++i)
  {
    ba[i] = n->number_[i];
  }

  Ptr<String> ret(new String(vm, static_cast<std::string>(ToHex(ba))));
  return ret;
}

template <typename T>
T UInt256Wrapper::ToPrimitive(VM * /*vm*/, Ptr<UInt256Wrapper> const &a)
{
  union
  {
    uint8_t bytes[sizeof(T)];
    T       value;
  } x;
  for (uint64_t i = 0; i < sizeof(T); ++i)
  {
    x.bytes[i] = a->number_[i];
  }

  return x.value;
}

void UInt256Wrapper::Bind(Module &module)
{
  module.CreateClassType<UInt256Wrapper>("UInt256")
      .CreateSerializeDefaultConstructor<uint64_t>(static_cast<uint64_t>(0))
      .CreateConstructor<uint64_t>()
      .CreateConstructor<Ptr<ByteArrayWrapper>>()
      .EnableOperator(Operator::Equal)
      .EnableOperator(Operator::NotEqual)
      .EnableOperator(Operator::LessThan)
      //        .EnableOperator(Operator::LessThanOrEqual)
      .EnableOperator(Operator::GreaterThan)
      //        .EnableOperator(Operator::GreaterThanOrEqual)
      //        .CreateMemberFunction("toBuffer", &UInt256Wrapper::ToBuffer)
      .CreateMemberFunction("increase", &UInt256Wrapper::Increase)
      //        .CreateMemberFunction("lessThan", &UInt256Wrapper::LessThan)
      .CreateMemberFunction("logValue", &UInt256Wrapper::LogValue)
      .CreateMemberFunction("toFloat64", &UInt256Wrapper::ToFloat64)
      .CreateMemberFunction("toInt32", &UInt256Wrapper::ToInt32)
      .CreateMemberFunction("size", &UInt256Wrapper::size);

  module.CreateFreeFunction("toString", &UInt256Wrapper::ToString);
  module.CreateFreeFunction("toUInt64", &UInt256Wrapper::ToPrimitive<uint64_t>);
  module.CreateFreeFunction("toInt64", &UInt256Wrapper::ToPrimitive<int64_t>);
  module.CreateFreeFunction("toUInt32", &UInt256Wrapper::ToPrimitive<uint32_t>);
  module.CreateFreeFunction("toInt32", &UInt256Wrapper::ToPrimitive<int32_t>);
}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, UInt256 data)
  : Object(vm, type_id)
  , number_(std::move(data))
{}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, byte_array::ByteArray const &data)
  : Object(vm, type_id)
  , number_(data)
{}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, uint64_t data)
  : Object(vm, type_id)
  , number_(data)
{}

Ptr<UInt256Wrapper> UInt256Wrapper::Constructor(VM *vm, TypeId type_id,
                                                Ptr<ByteArrayWrapper> const &ba)
{
  try
  {
    return new UInt256Wrapper(vm, type_id, ba->byte_array());
  }
  catch (std::runtime_error const &e)
  {
    vm->RuntimeError(e.what());
  }
  return nullptr;
}

Ptr<UInt256Wrapper> UInt256Wrapper::Constructor(VM *vm, TypeId type_id, uint64_t val)
{
  try
  {
    return new UInt256Wrapper(vm, type_id, val);
  }
  catch (std::runtime_error const &e)
  {
    vm->RuntimeError(e.what());
  }
  return nullptr;
}

double UInt256Wrapper::ToFloat64() const
{
  return ToDouble(number_);
}

int32_t UInt256Wrapper::ToInt32() const
{
  return static_cast<int32_t>(number_[0]);
}

double UInt256Wrapper::LogValue() const
{
  return Log(number_);
}

bool UInt256Wrapper::LessThan(Ptr<UInt256Wrapper> const &other) const
{
  return number_ < other->number_;
}

void UInt256Wrapper::Increase()
{
  ++number_;
}

UInt256Wrapper::SizeType UInt256Wrapper::size() const
{
  return number_.size();
}

vectorise::UInt<256> const &UInt256Wrapper::number() const
{
  return number_;
}

bool UInt256Wrapper::SerializeTo(serializers::ByteArrayBuffer &buffer)
{
  buffer << number_;
  return true;
}

bool UInt256Wrapper::DeserializeFrom(serializers::ByteArrayBuffer &buffer)
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

  variant["type"]  = GetUniqueId();
  variant["value"] = ToHex(value);
  return true;
}

bool UInt256Wrapper::FromJSON(JSONVariant const &variant)
{
  if (!variant.IsObject())
  {
    vm_->RuntimeError("JSON deserialisation of " + GetUniqueId() + " must be an object.");
    return false;
  }

  if (!variant.Has("type"))
  {
    vm_->RuntimeError("JSON deserialisation of " + GetUniqueId() + " must have field 'type'.");
    return false;
  }

  if (!variant.Has("value"))
  {
    vm_->RuntimeError("JSON deserialisation of " + GetUniqueId() + " must have field 'value'.");
    return false;
  }

  if (variant["type"].As<std::string>() != GetUniqueId())
  {
    vm_->RuntimeError("Field 'type' must be '" + GetUniqueId() + "'.");
    return false;
  }

  if (!variant["value"].IsString())
  {
    vm_->RuntimeError("Field 'value' must be a hex encoded string.");
    return false;
  }

  // TODO(issue 1262): Caller can't unambiguously detect whether the conversion failed or not
  auto const value = FromHex(variant["value"].As<byte_array::ConstByteArray>());

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

bool UInt256Wrapper::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<UInt256Wrapper> lhs = lhso;
  Ptr<UInt256Wrapper> rhs = rhso;
  return rhs->number_ < lhs->number_;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
