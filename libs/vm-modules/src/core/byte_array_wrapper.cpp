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
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"

#include <cstdint>

using namespace fetch::vm;
using namespace fetch::byte_array;

namespace fetch {
namespace vm_modules {

void ByteArrayWrapper::Bind(Module &module)
{
  module.CreateClassType<ByteArrayWrapper>("Buffer")
      .CreateSerializeDefaultConstructor(
          [](VM *vm, TypeId type_id) { return ByteArrayWrapper::Constructor(vm, type_id, 0); })
      .CreateConstructor(static_cast<Ptr<ByteArrayWrapper> (*)(VM *, TypeId, int32_t)>(
          &ByteArrayWrapper::Constructor))
      .CreateMemberFunction("copy", &ByteArrayWrapper::Copy)
      .CreateMemberFunction("toBase64", &ByteArrayWrapper::ToBase64)
      .CreateMemberFunction("fromBase64", &ByteArrayWrapper::FromBase64)
      //.CreateMemberFunction("toBase58", &ByteArrayWrapper::ToBase58)
      //.CreateMemberFunction("fromBase58", &ByteArrayWrapper::FromBase58)
      .CreateMemberFunction("toHex", &ByteArrayWrapper::ToHex)
      .CreateMemberFunction("fromHex", &ByteArrayWrapper::FromHex)
      .EnableOperator(Operator::LessThan)
      .EnableOperator(Operator::GreaterThan)
      .EnableOperator(Operator::LessThanOrEqual)
      .EnableOperator(Operator::GreaterThanOrEqual)
      .EnableOperator(Operator::Equal)
      .EnableOperator(Operator::NotEqual);
}

ByteArrayWrapper::ByteArrayWrapper(VM *vm, TypeId type_id, byte_array::ByteArray bytearray)
  : Object(vm, type_id)
  , byte_array_(std::move(bytearray))
{}

Ptr<ByteArrayWrapper> ByteArrayWrapper::Constructor(VM *vm, TypeId type_id, int32_t n)
{
  return Ptr<ByteArrayWrapper>{new ByteArrayWrapper(vm, type_id, ByteArray(std::size_t(n)))};
}

Ptr<ByteArrayWrapper> ByteArrayWrapper::Copy()
{
  return vm_->CreateNewObject<ByteArrayWrapper>(byte_array_.Copy());
}

Ptr<vm::String> ByteArrayWrapper::ToBase64()
{
  return Ptr<vm::String>{new String{vm_, static_cast<std::string>(byte_array_.ToBase64())}};
}

bool ByteArrayWrapper::FromBase64(fetch::vm::Ptr<vm::String> const &value_b64)
{
  if (!value_b64)
  {
    return false;
  }

  try
  {
    auto decoded_array{ByteArray{value_b64->string()}.FromBase64()};

    if (value_b64->Length() > 0 && decoded_array.empty())
    {
      return false;
    }

    byte_array_ = std::move(decoded_array);
    return true;
  }
  catch (std::runtime_error const &ex)
  {
    return false;
  }
}

fetch::vm::Ptr<vm::String> ByteArrayWrapper::ToHex()
{
  return Ptr<vm::String>{new String{vm_, static_cast<std::string>(byte_array_.ToHex())}};
}

bool ByteArrayWrapper::FromHex(fetch::vm::Ptr<vm::String> const &value_hex)
{
  if (!value_hex)
  {
    return false;
  }

  try
  {
    auto decoded_array{ByteArray{value_hex->string()}.FromHex()};

    if (value_hex->Length() > 0 && decoded_array.empty())
    {
      return false;
    }

    byte_array_ = std::move(decoded_array);
    return true;
  }
  catch (std::runtime_error const &ex)
  {
    return false;
  }
}

fetch::vm::Ptr<vm::String> ByteArrayWrapper::ToBase58()
{
  return Ptr<vm::String>{
      new String{vm_, static_cast<std::string>(byte_array::ToBase58(byte_array_))}};
}

bool ByteArrayWrapper::FromBase58(fetch::vm::Ptr<vm::String> const &value_b58)
{
  if (!value_b58)
  {
    return false;
  }

  try
  {
    auto decoded_array{byte_array::ToBase58(ConstByteArray{value_b58->string()})};

    if (value_b58->Length() > 0 && decoded_array.empty())
    {
      return false;
    }

    byte_array_ = std::move(decoded_array);
    return true;
  }
  catch (std::runtime_error const &ex)
  {
    return false;
  }
}

bool ByteArrayWrapper::IsEqual(fetch::vm::Ptr<Object> const &lhso,
                               fetch::vm::Ptr<Object> const &rhso)
{
  ByteArrayWrapper &l{static_cast<ByteArrayWrapper &>(*lhso)};
  ByteArrayWrapper &r{static_cast<ByteArrayWrapper &>(*rhso)};

  return l.byte_array_ == r.byte_array_;
}

bool ByteArrayWrapper::IsNotEqual(fetch::vm::Ptr<Object> const &lhso,
                                  fetch::vm::Ptr<Object> const &rhso)
{
  ByteArrayWrapper &l{static_cast<ByteArrayWrapper &>(*lhso)};
  ByteArrayWrapper &r{static_cast<ByteArrayWrapper &>(*rhso)};

  return l.byte_array_ != r.byte_array_;
}

bool ByteArrayWrapper::IsLessThan(fetch::vm::Ptr<Object> const &lhso,
                                  fetch::vm::Ptr<Object> const &rhso)
{
  ByteArrayWrapper &l{static_cast<ByteArrayWrapper &>(*lhso)};
  ByteArrayWrapper &r{static_cast<ByteArrayWrapper &>(*rhso)};

  return l.byte_array_ < r.byte_array_;
}

bool ByteArrayWrapper::IsLessThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                         fetch::vm::Ptr<Object> const &rhso)
{
  ByteArrayWrapper &l{static_cast<ByteArrayWrapper &>(*lhso)};
  ByteArrayWrapper &r{static_cast<ByteArrayWrapper &>(*rhso)};

  return l.byte_array_ <= r.byte_array_;
}

bool ByteArrayWrapper::IsGreaterThan(fetch::vm::Ptr<Object> const &lhso,
                                     fetch::vm::Ptr<Object> const &rhso)
{
  ByteArrayWrapper &l{static_cast<ByteArrayWrapper &>(*lhso)};
  ByteArrayWrapper &r{static_cast<ByteArrayWrapper &>(*rhso)};

  return l.byte_array_ > r.byte_array_;
}

bool ByteArrayWrapper::IsGreaterThanOrEqual(fetch::vm::Ptr<Object> const &lhso,
                                            fetch::vm::Ptr<Object> const &rhso)
{
  ByteArrayWrapper &l{static_cast<ByteArrayWrapper &>(*lhso)};
  ByteArrayWrapper &r{static_cast<ByteArrayWrapper &>(*rhso)};

  return l.byte_array_ >= r.byte_array_;
}

vm::ChargeAmount ByteArrayWrapper::IsEqualChargeEstimator(fetch::vm::Ptr<Object> const & /*lhso*/,
                                                          fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount ByteArrayWrapper::IsNotEqualChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount ByteArrayWrapper::IsLessThanChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount ByteArrayWrapper::IsLessThanOrEqualChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount ByteArrayWrapper::IsGreaterThanChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

vm::ChargeAmount ByteArrayWrapper::IsGreaterThanOrEqualChargeEstimator(
    fetch::vm::Ptr<Object> const & /*lhso*/, fetch::vm::Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ConstByteArray const &ByteArrayWrapper::byte_array() const
{
  return byte_array_;
}

bool ByteArrayWrapper::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  buffer << byte_array_;
  return true;
}

bool ByteArrayWrapper::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  buffer >> byte_array_;
  return true;
}

}  // namespace vm_modules
}  // namespace fetch
