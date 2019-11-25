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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "json/document.hpp"
#include "vm/array.hpp"
#include "vm/module.hpp"
#include "vm_modules/core/structured_data.hpp"
#include "vm_modules/math/tensor.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

namespace {

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::vm_modules::ByteArrayWrapper;
using fetch::vm_modules::math::UInt256Wrapper;

template <typename T>
constexpr uint8_t getTypeId() noexcept;

template <>
constexpr uint8_t getTypeId<vm::String>() noexcept
{
  return 's';
}

template <>
constexpr uint8_t getTypeId<vm::Address>() noexcept
{
  return 'a';
}

template <>
constexpr uint8_t getTypeId<ByteArrayWrapper>() noexcept
{
  return 'b';
}

template <>
constexpr uint8_t getTypeId<UInt256Wrapper>() noexcept
{
  return 'u';
}

template <typename T>
bool ExtractByteArrayRepresentigType(VM *vm, std::string const &key, ConstByteArray const &in_array,
                                     ConstByteArray &out_array)
{
  if (in_array.empty())
  {
    vm->RuntimeError("Unable to decode raw value for the " + key + " key");
    return false;
  }

  if (in_array[0] != getTypeId<T>())
  {
    vm->RuntimeError("Mismatching type for the " + key + " key");
    return false;
  }

  out_array = in_array.SubArray(1);
  return true;
}

template <typename T>
meta::EnableIf<vm::IsString<meta::Decay<T>>::value, Ptr<T>> FromByteArray(
    VM *vm, Ptr<String> const &name, ConstByteArray const &array)
{
  ConstByteArray value_array;
  if (!ExtractByteArrayRepresentigType<T>(vm, name->string(), array, value_array))
  {
    return Ptr<T>{};
  }

  return Ptr<T>{new T{vm, static_cast<std::string>(value_array)}};
}

template <typename T>
meta::EnableIf<IsAddress<meta::Decay<T>>::value, Ptr<T>> FromByteArray(VM *                  vm,
                                                                       Ptr<String> const &   name,
                                                                       ConstByteArray const &array)
{
  ConstByteArray value_array_base64;
  if (!ExtractByteArrayRepresentigType<T>(vm, name->string(), array, value_array_base64))
  {
    return Ptr<T>{};
  }

  if (value_array_base64.empty())
  {
    return vm->CreateNewObject<Address>();
  }

  auto const raw_address{value_array_base64.FromBase64()};
  if (!value_array_base64.empty() && raw_address.empty())
  {
    vm->RuntimeError("Unable to decode raw address value for " + name->string() + " item");
    return Ptr<T>{};
  }

  try
  {
    return vm->CreateNewObject<Address>(chain::Address{raw_address});
  }
  catch (std::runtime_error const &ex)
  {
    vm->RuntimeError("Unable to construct Address object from raw_address byte array for " +
                     name->string() + " item");
    return Ptr<T>{};
  }
}

template <typename T>
meta::EnableIf<std::is_same<ByteArrayWrapper, T>::value, Ptr<T>> FromByteArray(
    VM *vm, Ptr<String> const &name, ConstByteArray const &array)
{
  ConstByteArray value_array_base64;
  if (!ExtractByteArrayRepresentigType<T>(vm, name->string(), array, value_array_base64))
  {
    return Ptr<T>{};
  }

  ConstByteArray value_array{value_array_base64.FromBase64()};

  if (!value_array_base64.empty() and value_array.empty())
  {
    vm->RuntimeError("Unable to decode byte array value for " + name->string() + " item");
    return Ptr<T>{};
  }

  return vm->CreateNewObject<ByteArrayWrapper>(std::move(value_array));
}

template <typename T>
meta::EnableIf<std::is_same<UInt256Wrapper, T>::value, Ptr<T>> FromByteArray(
    VM *vm, Ptr<String> const &name, ConstByteArray const &array)
{
  ConstByteArray value_array_base64;
  if (!ExtractByteArrayRepresentigType<T>(vm, name->string(), array, value_array_base64))
  {
    return Ptr<T>{};
  }

  auto const value_array{value_array_base64.FromBase64()};
  if (!value_array_base64.empty() && value_array.empty())
  {
    vm->RuntimeError("Unable to decode UInt256 value for " + name->string() + " item");
    return Ptr<T>{};
  }

  return vm->CreateNewObject<UInt256Wrapper>(value_array);
}

ByteArray ToByteArray(String const &str)
{
  ByteArray encoded_array;
  encoded_array.Append(getTypeId<String>(), str.string());
  return encoded_array;
}

ByteArray ToByteArray(Address const &addr)
{
  ByteArray encoded_array;
  encoded_array.Append(getTypeId<Address>(), addr.address().address().ToBase64());
  return encoded_array;
}

ByteArray ToByteArray(ByteArrayWrapper const &byte_array)
{
  ByteArray encoded_array;
  encoded_array.Append(getTypeId<ByteArrayWrapper>(), byte_array.byte_array().ToBase64());
  return encoded_array;
}

ByteArray ToByteArray(UInt256Wrapper const &big_number)
{
  ByteArray encoded_array;
  encoded_array.Append(
      getTypeId<UInt256Wrapper>(),
      byte_array::ToBase64(big_number.number().pointer(), big_number.number().TrimmedSize()));
  return encoded_array;
}

template <typename T>
Ptr<Array<T>> CreateNewPrimitiveArray(VM *vm, std::vector<T> &&items)
{
  Ptr<Array<T>> array{
      new Array<T>(vm, vm->GetTypeId<IArray>(), vm->GetTypeId<T>(), int32_t(items.size()))};
  array->elements = std::move(items);

  return array;
}

}  // namespace

void StructuredData::Bind(Module &module)
{
  module.CreateClassType<StructuredData>("StructuredData")
      .CreateConstructor(&StructuredData::Constructor)
      // Getters
      .CreateMemberFunction("getInt32", &StructuredData::GetPrimitive<int32_t>)
      .CreateMemberFunction("getInt64", &StructuredData::GetPrimitive<int64_t>)
      .CreateMemberFunction("getUInt32", &StructuredData::GetPrimitive<uint32_t>)
      .CreateMemberFunction("getUInt64", &StructuredData::GetPrimitive<uint64_t>)
      .CreateMemberFunction("getFloat32", &StructuredData::GetPrimitive<float>)
      .CreateMemberFunction("getFloat64", &StructuredData::GetPrimitive<double>)
      .CreateMemberFunction("getString", &StructuredData::GetObject<String>)
      .CreateMemberFunction("getAddress", &StructuredData::GetObject<Address>)
      .CreateMemberFunction("getBuffer", &StructuredData::GetObject<ByteArrayWrapper>)
      .CreateMemberFunction("getUInt256", &StructuredData::GetObject<UInt256Wrapper>)
      .CreateMemberFunction("getArrayInt32", &StructuredData::GetArray<int32_t>)
      .CreateMemberFunction("getArrayInt64", &StructuredData::GetArray<int64_t>)
      .CreateMemberFunction("getArrayUInt32", &StructuredData::GetArray<uint32_t>)
      .CreateMemberFunction("getArrayUInt64", &StructuredData::GetArray<uint64_t>)
      .CreateMemberFunction("getArrayFloat32", &StructuredData::GetArray<float>)
      .CreateMemberFunction("getArrayFloat64", &StructuredData::GetArray<double>)
      // Setters
      .CreateMemberFunction("set", &StructuredData::SetArray<int32_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<int64_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<uint32_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<uint64_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<float>)
      .CreateMemberFunction("set", &StructuredData::SetArray<double>)
      .CreateMemberFunction("set", &StructuredData::SetObject<String>)
      .CreateMemberFunction("set", &StructuredData::SetObject<Address>)
      .CreateMemberFunction("set", &StructuredData::SetObject<ByteArrayWrapper>)
      .CreateMemberFunction("set", &StructuredData::SetObject<UInt256Wrapper>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<int32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<int64_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<uint32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<uint64_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<float>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<double>);

  // add array support?
  module.GetClassInterface<IArray>().CreateInstantiationType<Array<Ptr<StructuredData>>>();
}

Ptr<StructuredData> StructuredData::Constructor(VM *vm, TypeId type_id)
{
  return Ptr<StructuredData>{new StructuredData(vm, type_id)};
}

vm::Ptr<StructuredData> StructuredData::ConstructorFromVariant(vm::VM *vm, vm::TypeId type_id,
                                                               variant::Variant const &data)
{
  Ptr<StructuredData> structured_data{};

  try
  {
    if (!data.IsObject())
    {
      vm->RuntimeError("Unable to parse input variant for structured data");
    }
    else
    {
      // create the structured data
      structured_data = Constructor(vm, type_id);

      // copy the data across
      structured_data->contents_ = data;
    }
  }
  catch (std::exception const &ex)
  {
    vm->RuntimeError(std::string{"Internal error creating structured data: "} + ex.what());
  }

  return structured_data;
}

StructuredData::StructuredData(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{}

bool StructuredData::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  bool success{false};

  try
  {
    std::ostringstream oss;
    oss << contents_;

    buffer << ConstByteArray{oss.str()};

    success = true;
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Error extracting from JSON: "} + ex.what());
  }

  return success;
}

bool StructuredData::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  bool success{false};

  try
  {
    ConstByteArray data;
    buffer >> data;

    json::JSONDocument doc{data};
    contents_ = doc.root();

    success = true;
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Error extracting from JSON: "} + ex.what());
  }

  return success;
}

bool StructuredData::ToJSON(JSONVariant &variant)
{
  bool success{false};

  try
  {
    variant = contents_;
    success = true;
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Error generating JSON: "} + ex.what());
  }

  return success;
}

bool StructuredData::FromJSON(JSONVariant const &variant)
{
  bool success{false};

  try
  {
    contents_ = variant;
    success   = true;
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Error extracting from JSON: "} + ex.what());
  }

  return success;
}

bool StructuredData::Has(Ptr<String> const &s)
{
  return contents_.Has(s->string());
}

template <typename T>
StructuredData::IfIsSupportedRefType<T, Ptr<T>> StructuredData::GetObject(
    vm::Ptr<vm::String> const &s)
{
  try
  {
    if (Has(s))
    {
      auto const v_item{contents_[s->string()]};
      if (v_item.IsNull())
      {
        return Ptr<T>{};
      }

      return FromByteArray<T>(vm_, s, v_item.As<ConstByteArray>());
    }

    vm_->RuntimeError("Unable to look up item" +
                      (s ? ("for the \"" + s->string() + "\" key") : std::string{}) +
                      " in the StructuredData object");
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(e.what());
  }

  return Ptr<T>{};
}

template <typename T>
T StructuredData::GetPrimitive(Ptr<String> const &s)
{
  T ret{0};

  try
  {
    // check that the value exists
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to look up item: " + s->string());
    }
    else
    {
      ret = contents_[s->string()].As<T>();
    }
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(e.what());
  }

  return ret;
}

template <typename T>
Ptr<Array<T>> StructuredData::GetArray(Ptr<String> const &s)
{
  Ptr<Array<T>> ret{};

  try
  {
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to look up item: " + s->string());
    }
    else
    {
      auto const &value_array = contents_[s->string()];

      if (!value_array.IsArray())
      {
        vm_->RuntimeError("Internal element is not an array");
      }
      else
      {
        // create and preallocate the vector of elements
        std::vector<T> elements;
        elements.resize(value_array.size());

        // copy each of the elements
        for (std::size_t i = 0; i < value_array.size(); ++i)
        {
          elements[i] = value_array[i].As<T>();
        }

        ret = CreateNewPrimitiveArray(vm_, std::move(elements));
      }
    }
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string{"Internal error: "} + e.what());
  }

  return ret;
}

template <typename T>
void StructuredData::SetPrimitive(Ptr<String> const &s, T value)
{
  try
  {
    contents_[s->string()] = value;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string{"Internal error setting structured value: "} + e.what());
  }
}

template <typename T>
void StructuredData::SetArray(Ptr<String> const &s, Ptr<Array<T>> const &arr)
{
  try
  {
    auto &values = contents_[s->string()];

    // update the value to be an array
    values = variant::Variant::Array(arr->elements.size());

    // add the elements into the array
    for (std::size_t i = 0; i < arr->elements.size(); ++i)
    {
      values[i] = arr->elements[i];
    }
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError("Unable to set array of variables");
  }
}

template <typename T>
StructuredData::IfIsSupportedRefType<T> StructuredData::SetObject(Ptr<String> const &s,
                                                                  Ptr<T> const &     value)
{
  try
  {
    if (value)
    {
      contents_[s->string()] = ToByteArray(*value);
    }
    else
    {
      contents_[s->string()] = variant::Variant::Null();
    }
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Internal error setting item" +
                                  (s ? (" for the \"" + s->string() + "\" key") : std::string{}) +
                                  " in to StructuredData object: "} +
                      ex.what());
  }
}

}  // namespace vm_modules
}  // namespace fetch
