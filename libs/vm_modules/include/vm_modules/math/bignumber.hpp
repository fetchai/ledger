#pragma once
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

#include <algorithm>
#include <cstdint>

namespace fetch {
namespace vm_modules {
namespace math {

class UInt256Wrapper : public fetch::vm::Object
{
public:
  using SizeType = uint64_t;
  using UInt256  = vectorise::UInt<256>;

  template <typename T>
  using Ptr    = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  UInt256Wrapper()          = delete;
  virtual ~UInt256Wrapper() = default;

  static fetch::vm::Ptr<fetch::vm::String> ToString(fetch::vm::VM *vm, Ptr<UInt256Wrapper> const &n)
  {
    byte_array::ByteArray ba(32);
    for (uint64_t i = 0; i < 32; ++i)
    {
      ba[i] = n->number_[i];
    }

    fetch::vm::Ptr<fetch::vm::String> ret(
        new fetch::vm::String(vm, static_cast<std::string>(ToHex(ba))));
    return ret;
  }

  template <typename T>
  static T ToPrimitive(fetch::vm::VM * /*vm*/, Ptr<UInt256Wrapper> const &a)
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

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<UInt256Wrapper>("UInt256")
        .CreateSerializeDefaultConstuctor<uint64_t>(static_cast<uint64_t>(0))
        .CreateConstuctor<uint64_t>()
        .CreateConstuctor<Ptr<vm::String>>()
        .CreateConstuctor<Ptr<ByteArrayWrapper>>()
        .EnableOperator(vm::Operator::Equal)
        .EnableOperator(vm::Operator::NotEqual)
        .EnableOperator(vm::Operator::LessThan)
        //        .EnableOperator(vm::Operator::LessThanOrEqual)
        .EnableOperator(vm::Operator::GreaterThan)
        //        .EnableOperator(vm::Operator::GreaterThanOrEqual)
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

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, UInt256 data)
    : fetch::vm::Object(vm, type_id)
    , number_(std::move(data))
  {}

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray const &data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  UInt256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint64_t data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                    fetch::vm::Ptr<ByteArrayWrapper> const &ba)
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

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                    fetch::vm::Ptr<vm::String> const &ba)
  {
    try
    {
      return new UInt256Wrapper(vm, type_id, ba->str);
    }
    catch (std::runtime_error const &e)
    {
      vm->RuntimeError(e.what());
    }
    return nullptr;
  }

  static fetch::vm::Ptr<UInt256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                    uint64_t val)
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

  double ToFloat64()
  {
    return ToDouble(number_);
  }

  int32_t ToInt32()
  {
    union
    {
      uint8_t bytes[4];
      int32_t value;
    } x;
    x.bytes[0] = number_[0];
    x.bytes[1] = number_[1];
    x.bytes[2] = number_[2];
    x.bytes[3] = number_[3];
    return x.value;
  }

  double LogValue()
  {
    return Log(number_);
  }

  /*
  fetch::vm::Ptr<ByteArrayWrapper> ToBuffer()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(number_.Copy());
  }
  */

  bool LessThan(Ptr<UInt256Wrapper> const &other)
  {
    return number_ < other->number_;
  }

  void Increase()
  {
    ++number_;
  }

  SizeType size() const
  {
    return number_.size();
  }

  vectorise::UInt<256> const &number() const
  {
    return number_;
  }

  bool SerializeTo(serializers::ByteArrayBuffer &buffer) override
  {
    buffer << number_;
    return true;
  }

  bool DeserializeFrom(serializers::ByteArrayBuffer &buffer) override
  {
    buffer >> number_;
    return true;
  }

  bool ToJSON(vm::JSONVariant &variant) override
  {
    variant = vm::JSONVariant::Object();
    byte_array::ByteArray value(32);
    for (uint64_t i = 0; i < 32; ++i)
    {
      value[i] = number_[i];
    }

    variant["type"]  = GetUniqueId();
    variant["value"] = ToHex(value);
    return true;
  }

  bool FromJSON(vm::JSONVariant const &variant) override
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

  bool IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<UInt256Wrapper> lhs = lhso;
    Ptr<UInt256Wrapper> rhs = rhso;
    return lhs->number_ == rhs->number_;
  }

  bool IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<UInt256Wrapper> lhs = lhso;
    Ptr<UInt256Wrapper> rhs = rhso;
    return lhs->number_ != rhs->number_;
  }

  bool IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<UInt256Wrapper> lhs = lhso;
    Ptr<UInt256Wrapper> rhs = rhso;
    return lhs->number_ < rhs->number_;
  }

  bool IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override
  {
    Ptr<UInt256Wrapper> lhs = lhso;
    Ptr<UInt256Wrapper> rhs = rhso;
    return rhs->number_ < lhs->number_;
  }

private:
  vectorise::UInt<256> number_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
