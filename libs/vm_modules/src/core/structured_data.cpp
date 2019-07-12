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
#include "core/json/document.hpp"
#include "vm/module.hpp"
#include "vm_modules/core/structured_data.hpp"

#include <sstream>

namespace fetch {
namespace vm_modules {
namespace {

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ToBase64;
using fetch::byte_array::FromBase64;

template <typename T>
vm::Ptr<vm::Array<T>> CreateNewPrimitiveArray(vm::VM *vm, std::vector<T> &&items)
{
  vm::Ptr<vm::Array<T>> array =
      new vm::Array<T>(vm, vm->GetTypeId<vm::IArray>(), vm->GetTypeId<T>(), int32_t(items.size()));
  array->elements = std::move(items);

  return array;
}

}  // namespace

void StructuredData::Bind(vm::Module &module)
{
  auto interface = module.CreateClassType<StructuredData>("StructuredData");

  interface.CreateConstuctor<>()
    // Getters
      .CreateMemberFunction("getInt32", &StructuredData::GetPrimitive<int32_t>)
      .CreateMemberFunction("getInt64", &StructuredData::GetPrimitive<int64_t>)
      .CreateMemberFunction("getUInt32", &StructuredData::GetPrimitive<uint32_t>)
      .CreateMemberFunction("getUInt64", &StructuredData::GetPrimitive<uint64_t>)
      .CreateMemberFunction("getFloat32", &StructuredData::GetPrimitive<float>)
      .CreateMemberFunction("getFloat64", &StructuredData::GetPrimitive<double>)
      .CreateMemberFunction("getString", &StructuredData::GetString)
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
      .CreateMemberFunction("set", &StructuredData::SetString)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<int32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<int64_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<uint32_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<uint64_t>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<float>)
      .CreateMemberFunction("set", &StructuredData::SetPrimitive<double>);

  // add array support?
  module.GetClassInterface<vm::IArray>()
      .CreateInstantiationType<vm::Array<vm::Ptr<StructuredData>>>();
}

vm::Ptr<StructuredData> StructuredData::Constructor(vm::VM *vm, vm::TypeId type_id)
{
  return new StructuredData(vm, type_id);
}

vm::Ptr<StructuredData> StructuredData::Constructor(vm::VM *vm, vm::TypeId type_id,
                                                    variant::Variant const &data)
{
  vm::Ptr<StructuredData> structured_data{};

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

StructuredData::StructuredData(vm::VM *vm, vm::TypeId type_id)
  : vm::Object(vm, type_id)
{}

bool StructuredData::SerializeTo(vm::ByteArrayBuffer &buffer)
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

bool StructuredData::DeserializeFrom(vm::ByteArrayBuffer &buffer)
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

bool StructuredData::ToJSON(vm::JSONVariant &variant)
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

bool StructuredData::FromJSON(vm::JSONVariant const &variant)
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

bool StructuredData::Has(vm::Ptr<vm::String> const &s)
{
  return contents_.Has(s->str);
}

vm::Ptr<vm::String> StructuredData::GetString(vm::Ptr<vm::String> const &s)
{
  std::string ret;

  try
  {
    // check that the value exists
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to lookup item: " + s->str);
    }
    else
    {
      auto const decoded = FromBase64(contents_[s->str].As<ConstByteArray>());
      ret                = static_cast<std::string>(decoded);
    }
  }
  catch (std::runtime_error const &e)
  {
    vm_->RuntimeError(e.what());
  }

  return new vm::String(vm_, ret);
}

template <typename T>
T StructuredData::GetPrimitive(vm::Ptr<vm::String> const &s)
{
  T ret{0};

  try
  {
    // check that the value exists
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to lookup item: " + s->str);
    }
    else
    {
      ret = contents_[s->str].As<T>();
    }
  }
  catch (std::runtime_error const &e)
  {
    vm_->RuntimeError(e.what());
  }

  return ret;
}

template <typename T>
vm::Ptr<vm::Array<T>> StructuredData::GetArray(vm::Ptr<vm::String> const &s)
{
  vm::Ptr<vm::Array<T>> ret{};

  try
  {
    if (!Has(s))
    {
      vm_->RuntimeError("Unable to lookup item: " + s->str);
    }
    else
    {
      auto const &value_array = contents_[s->str];

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
void StructuredData::SetPrimitive(vm::Ptr<vm::String> const &s, T value)
{
  try
  {
    contents_[s->str] = value;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string{"Internal error setting structured value: "} + e.what());
  }
}

template <typename T>
void StructuredData::SetArray(vm::Ptr<vm::String> const &s, vm::Ptr<vm::Array<T>> const &arr)
{
  try
  {
    auto &values = contents_[s->str];

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

void StructuredData::SetString(vm::Ptr<vm::String> const &s, vm::Ptr<vm::String> const &value)
{
  try
  {
    contents_[s->str] = ToBase64(value->str);
  }
  catch (std::exception const &ex)
  {
    vm_->RuntimeError(std::string{"Internal error setting string: "} + ex.what());
  }
}

}  // namespace vm_modules
}  // namespace fetch
