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
#include "meta/type_traits.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
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
using fetch::byte_array::ToBase64;
using fetch::byte_array::FromBase64;
using fetch::meta::EnableIfNotSame;

template <typename T>
Ptr<Array<T>> CreateNewPrimitiveArray(VM *vm, std::vector<T> &&items)
{
  Ptr<Array<T>> array{
      new Array<T>(vm, vm->GetTypeId<IArray>(), vm->GetTypeId<T>(), int32_t(items.size()))};
  array->elements = std::move(items);

  return array;
}

template <typename T>
Ptr<Array<Ptr<T>>> CreateNewPtrArray(VM *vm, std::vector<Ptr<T>> &&items)
{
  Ptr<Array<Ptr<T>>> array{
      new Array<Ptr<T>>(vm, vm->GetTypeId<IArray>(), vm->GetTypeId<T>(), int32_t(items.size()))};
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
      .CreateMemberFunction("getFixed32", &StructuredData::GetPrimitive<fixed_point::fp32_t>)
      .CreateMemberFunction("getFixed64", &StructuredData::GetPrimitive<fixed_point::fp64_t>)
      .CreateMemberFunction("getFixed128", &StructuredData::GetFixed128)
      .CreateMemberFunction("getString", &StructuredData::GetString)
      .CreateMemberFunction("getArrayInt32", &StructuredData::GetArray<int32_t>)
      .CreateMemberFunction("getArrayInt64", &StructuredData::GetArray<int64_t>)
      .CreateMemberFunction("getArrayUInt32", &StructuredData::GetArray<uint32_t>)
      .CreateMemberFunction("getArrayUInt64", &StructuredData::GetArray<uint64_t>)
      .CreateMemberFunction("getArrayFloat32", &StructuredData::GetArray<float>)
      .CreateMemberFunction("getArrayFloat64", &StructuredData::GetArray<double>)
      .CreateMemberFunction("getArrayFixed32", &StructuredData::GetArray<fixed_point::fp32_t>)
      .CreateMemberFunction("getArrayFixed64", &StructuredData::GetArray<fixed_point::fp64_t>)
      .CreateMemberFunction("getArrayFixed128", &StructuredData::GetFixed128Array)
      // Setters
      .CreateMemberFunction("set", &StructuredData::SetArray<int32_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<int64_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<uint32_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<uint64_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<float>)
      .CreateMemberFunction("set", &StructuredData::SetArray<double>)
      .CreateMemberFunction("set", &StructuredData::SetArray<fixed_point::fp32_t>)
      .CreateMemberFunction("set", &StructuredData::SetArray<fixed_point::fp64_t>)
      .CreateMemberFunction("set", &StructuredData::SetFixed128Array)
      .CreateMemberFunction("set", &StructuredData::SetString)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<int32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<int64_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<uint32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<uint64_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<float>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<double>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<fixed_point::fp32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<fixed_point::fp64_t>)
      .CreateMemberFunction("set", &StructuredData::SetFixed128);

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

Ptr<String> StructuredData::GetString(Ptr<String> const &s)
{
  std::string ret;

  try
  {
    // check that the value exists
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to look up item: " + s->string());
    }
    else
    {
      auto const decoded = FromBase64(contents_[s->string()].As<ConstByteArray>());
      ret                = static_cast<std::string>(decoded);
    }
  }
  catch (std::runtime_error const &e)
  {
    vm_->RuntimeError(e.what());
  }

  return Ptr<String>{new String(vm_, ret)};
}

Ptr<Fixed128> StructuredData::GetFixed128(Ptr<String> const &s)
{
  fixed_point::fp128_t ret;

  try
  {
    // check that the value exists
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to look up item: " + s->string());
    }
    else
    {
      auto const decoded = FromBase64(contents_[s->string()].As<ConstByteArray>());
      int128_t number = *reinterpret_cast<int128_t const *>(decoded.pointer());
      ret = fixed_point::fp128_t::FromBase(number);
    }
  }
  catch (std::runtime_error const &e)
  {
    vm_->RuntimeError(e.what());
  }

  return Ptr<Fixed128>{new Fixed128(vm_, ret)};
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
  catch (std::runtime_error const &e)
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

Ptr<Array<Ptr<Fixed128>>> StructuredData::GetFixed128Array(Ptr<String> const &s)
{
  Ptr<Array<Ptr<Fixed128>>> ret{};

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
        std::vector<Ptr<Fixed128>> elements;
        elements.resize(value_array.size());

        // copy each of the elements
        for (std::size_t i = 0; i < value_array.size(); ++i)
        {
          elements[i] = Ptr<Fixed128>(new Fixed128(vm_, value_array[i].As<fixed_point::fp128_t>()));
        }

        ret = CreateNewPtrArray<Fixed128>(vm_, std::move(elements));
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
EnableIfNotSame<T, Ptr<vm::Fixed128>> StructuredData::SetArray(Ptr<String> const &s, Ptr<Array<T>> const &arr)
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

void StructuredData::SetFixed128Array(Ptr<String> const &s, Ptr<Array<Ptr<Fixed128>>> const &arr)
{
  try
  {
    auto &values = contents_[s->string()];

    // update the value to be an array
    values = variant::Variant::Array(arr->elements.size());

    // add the elements into the array
    for (std::size_t i = 0; i < arr->elements.size(); ++i)
    {
      Ptr<Fixed128> element = arr->elements[i];
      Ptr<String> value;
      SetFixed128(value, element);
      values[i] = value->string();
    }
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError("Unable to set array of variables");
  }
}

void StructuredData::SetString(Ptr<String> const &s, Ptr<String> const &value)
{
  try
  {
    contents_[s->string()] = ToBase64(value->string());
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Internal error setting string: "} + ex.what());
  }
}

void StructuredData::SetFixed128(Ptr<String> const &s, Ptr<Fixed128> const &value)
{
  try
  {
    ConstByteArray buf(reinterpret_cast<uint8_t const *>(&value->data), sizeof(int128_t));
    contents_[s->string()] = ToBase64(buf);
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Internal error setting string: "} + ex.what());
  }
}

}  // namespace vm_modules
}  // namespace fetch
