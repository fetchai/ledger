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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "vectorise/uint/uint.hpp"

#include "vm/module.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/math/bignumber.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace fetch {
namespace vm_modules {
namespace math {

using namespace fetch::vm;

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
        new UInt256Wrapper(vm, type_id, ba->byte_array(), platform::Endian::BIG)};
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
      .EnableOperator(Operator::Add)
      .EnableOperator(Operator::InplaceAdd)
      .EnableOperator(Operator::Subtract)
      .EnableOperator(Operator::InplaceSubtract)
      .EnableOperator(Operator::Multiply)
      .EnableOperator(Operator::Divide)
      .EnableOperator(Operator::InplaceMultiply)
      .EnableOperator(Operator::InplaceDivide)
      .CreateMemberFunction("copy", &UInt256Wrapper::Copy)
      .CreateMemberFunction("size", &UInt256Wrapper::size);

  module.CreateFreeFunction("toString", &ToString);
  module.CreateFreeFunction("toBuffer", [](VM *vm, Ptr<UInt256Wrapper> const &x) {
    return vm->CreateNewObject<ByteArrayWrapper>(
        x->number().As<byte_array::ByteArray>(platform::Endian::BIG, true));
  });
  module.CreateFreeFunction("toUInt64", &ToInteger<uint64_t>);
  module.CreateFreeFunction("toInt64", &ToInteger<int64_t>);
  module.CreateFreeFunction("toUInt32", &ToInteger<uint32_t>);
  module.CreateFreeFunction("toInt32", &ToInteger<int32_t>);
}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, UInt256Wrapper::UInt256 data)
  : Object(vm, type_id)
  , number_(std::move(data))
{}

UInt256Wrapper::UInt256Wrapper(VM *vm, UInt256 data)
  : UInt256Wrapper{vm, TypeIds::UInt256, std::move(data)}
{}

UInt256Wrapper::UInt256Wrapper(VM *vm, TypeId type_id, byte_array::ConstByteArray const &data,
                               platform::Endian endianess_of_input_data)
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

vm::Ptr<UInt256Wrapper> UInt256Wrapper::Copy() const
{
  return Ptr<UInt256Wrapper>{new UInt256Wrapper(this->vm_, UInt256{number_})};
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
  catch (std::exception const &)
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

void UInt256Wrapper::Add(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  if (lhs->IsTemporary())
  {
    lhs->number_ += rhs->number_;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->number_ += lhs->number_;
    lhso = rhs;
    return;
  }

  Ptr<UInt256Wrapper> n{new UInt256Wrapper{vm_, GetTypeId(), lhs->number_ + rhs->number_}};
  lhso = std::move(n);
}

void UInt256Wrapper::Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  if (lhs->IsTemporary())
  {
    lhs->number_ -= rhs->number_;
    return;
  }

  Ptr<UInt256Wrapper> n(new UInt256Wrapper(vm_, lhs->number_ - rhs->number_));
  lhso = std::move(n);
}

void UInt256Wrapper::InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  lhs->number_ += rhs->number_;
}

void UInt256Wrapper::InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  lhs->number_ -= rhs->number_;
}

void UInt256Wrapper::Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  if (lhs->IsTemporary())
  {
    lhs->number_ *= rhs->number_;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->number_ *= lhs->number_;
    lhso = rhs;
    return;
  }
  Ptr<UInt256Wrapper> n(new UInt256Wrapper(vm_, lhs->number_ * rhs->number_));
  lhso = std::move(n);
}

void UInt256Wrapper::InplaceMultiply(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  lhs->number_ *= rhs->number_;
}

void UInt256Wrapper::Divide(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  if (rhs->number_ == UInt256::_0)
  {
    vm_->RuntimeError("UInt256Wrapper::Divide runtime error : division by zero.");
    return;
  }
  if (lhs->IsTemporary())
  {
    lhs->number_ /= rhs->number_;
    return;
  }

  Ptr<UInt256Wrapper> n(new UInt256Wrapper(vm_, lhs->number_ / rhs->number_));
  lhso = std::move(n);
}

void UInt256Wrapper::InplaceDivide(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  try
  {
    lhs->number_ /= rhs->number_;
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string("UInt256Wrapper::InplaceDivide runtime error: ") + ex.what());
    return;
  }
}

bool UInt256Wrapper::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  return lhs->number_ == rhs->number_;
}

bool UInt256Wrapper::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  return lhs->number_ != rhs->number_;
}

bool UInt256Wrapper::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  return lhs->number_ < rhs->number_;
}

bool UInt256Wrapper::IsLessThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                       fetch::vm::Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  return lhs->number_ <= rhs->number_;
}

bool UInt256Wrapper::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  return rhs->number_ < lhs->number_;
}

bool UInt256Wrapper::IsGreaterThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                          fetch::vm::Ptr<Object> const &rhso)
{
  auto &lhs = static_cast<Ptr<UInt256Wrapper> const &>(lhso);
  auto &rhs = static_cast<Ptr<UInt256Wrapper> const &>(rhso);
  return rhs->number_ <= lhs->number_;
}

vm::ChargeAmount UInt256Wrapper::AddChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                    fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::InplaceAddChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                           fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::SubtractChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                         fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::InplaceSubtractChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::MultiplyChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                         fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::InplaceMultiplyChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::DivideChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                       fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::InplaceDivideChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::IsEqualChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                        fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::IsNotEqualChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                           fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::IsLessThanChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                           fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::IsLessThanOrEqualChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::IsGreaterThanChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount UInt256Wrapper::IsGreaterThanOrEqualChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
