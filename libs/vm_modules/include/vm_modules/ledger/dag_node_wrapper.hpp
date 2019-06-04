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

#include "vm/analyser.hpp"
#include "vm/common.hpp"

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "ledger/dag/dag.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"

namespace fetch {
namespace vm_modules {
template <typename T>
vm::Ptr<vm::Array<T>> CreateNewPrimitiveArray(vm::VM *vm, std::vector<T> items)
{
  vm::Ptr<vm::Array<T>> array =
      new vm::Array<T>(vm, vm->GetTypeId<vm::IArray>(), vm->GetTypeId<T>(), int32_t(items.size()));
  std::size_t idx = 0;

  for (auto const &e : items)
  {
    array->elements[idx++] = e;
  }

  return array;
}

class DAGNodeWrapper : public fetch::vm::Object
{
public:
  using Node = ledger::DAGNode;

  DAGNodeWrapper()           = delete;
  ~DAGNodeWrapper() override = default;

  static void Bind(vm::Module & module)
  {
    auto interface = module.CreateClassType<DAGNodeWrapper>("DAGNode");


    interface.CreateConstuctor<>()
        .CreateMemberFunction("owner", &DAGNodeWrapper::owner)
        .CreateMemberFunction("has", &DAGNodeWrapper::Has)
        // Getters
        .CreateMemberFunction("getNumber", &DAGNodeWrapper::GetNumber)
        .CreateMemberFunction("getArrayFloat32", &DAGNodeWrapper::GetArray<float>)
        .CreateMemberFunction("getArrayFloat64", &DAGNodeWrapper::GetArray<double>)
        .CreateMemberFunction("getArrayInt32", &DAGNodeWrapper::GetArray<int32_t>)
        .CreateMemberFunction("getArrayInt64", &DAGNodeWrapper::GetArray<int64_t>)
        .CreateMemberFunction("getArrayUInt32", &DAGNodeWrapper::GetArray<uint32_t>)
        .CreateMemberFunction("getArrayUInt64", &DAGNodeWrapper::GetArray<uint64_t>)
        .CreateMemberFunction("getFloat32", &DAGNodeWrapper::GetPrimitive<float>)
        .CreateMemberFunction("getFloat64", &DAGNodeWrapper::GetPrimitive<double>)
        .CreateMemberFunction("getInt32", &DAGNodeWrapper::GetPrimitive<int32_t>)
        .CreateMemberFunction("getInt64", &DAGNodeWrapper::GetPrimitive<int64_t>)
        .CreateMemberFunction("getUInt32", &DAGNodeWrapper::GetPrimitive<uint32_t>)
        .CreateMemberFunction("getUInt64", &DAGNodeWrapper::GetPrimitive<uint64_t>)
        .CreateMemberFunction("getString", &DAGNodeWrapper::GetString)
        .CreateMemberFunction("getBuffer", &DAGNodeWrapper::GetBuffer)
        //      .CreateMemberFunction("GetStringArray", &DAGNodeWrapper::GetArrayString)
        // Setters
//              .CreateMemberFunction("set", &DAGNodeWrapper::SetStringArray)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetBuffer)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetString)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetArray<float>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetArray<double>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetArray<int32_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetArray<int64_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetArray<uint32_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetArray<uint64_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetPrimitive<float>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetPrimitive<double>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetPrimitive<int32_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetPrimitive<int64_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetPrimitive<uint32_t>)
        .CreateMemberFunction("set", &DAGNodeWrapper::SetPrimitive<uint64_t>);
  }

  DAGNodeWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Node const &node)
    : fetch::vm::Object(vm, type_id)
    , node_(node)
  {
    try
    {
      contents_.Parse(node_.contents);
    }
    catch (std::exception const &e)
    {
      vm->RuntimeError(e.what());
    }
  }

  DAGNodeWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {
    // VM can by default only produce DATA
    node_.type = Node::DATA;
  }

  static fetch::vm::Ptr<DAGNodeWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new DAGNodeWrapper(vm, type_id);
  }

  fetch::vm::Ptr<ByteArrayWrapper> owner()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(node_.identity.identifier());
  }

  bool Has(vm::Ptr<vm::String> const &s)
  {
    return contents_.Has(s->str);
  }

  vm::Ptr<vm::String> GetString(vm::Ptr<vm::String> const &s)
  {
    std::string ret;
    try
    {
      ret = static_cast<std::string>(
          byte_array::FromBase64(contents_[s->str].As<byte_array::ConstByteArray>()));
    }
    catch (std::runtime_error const &e)
    {
      vm_->RuntimeError(e.what());
    }
    return new fetch::vm::String(vm_, ret);
  }

  vm::Ptr<ByteArrayWrapper> GetBuffer(vm::Ptr<vm::String> const &s)
  {
    byte_array::ByteArray ret;
    try
    {
      ret = byte_array::FromBase64(contents_[s->str].As<byte_array::ConstByteArray>());
    }
    catch (std::runtime_error const &e)
    {
      vm_->RuntimeError(e.what());
    }
    return vm_->CreateNewObject<ByteArrayWrapper>(ret);
  }

  double GetNumber(vm::Ptr<vm::String> const &s)
  {
    double ret = 0;
    try
    {
      ret = contents_[s->str].As<double>();
    }
    catch (std::runtime_error const &e)
    {
      try
      {
        ret = static_cast<double>(contents_[s->str].As<int64_t>());
      }
      catch (std::runtime_error const &e)
      {
        vm_->RuntimeError(e.what());
      }
    }
    return ret;
  }

  template <typename T>
  T GetPrimitive(vm::Ptr<vm::String> const &s)
  {
    T ret = 0;

    try
    {
      ret = contents_[s->str].As<T>();
    }
    catch (std::runtime_error const &e)
    {
      vm_->RuntimeError(e.what());
    }
    return ret;
  }

  template <typename T>
  vm::Ptr<vm::Array<T>> GetArray(vm::Ptr<vm::String> const &s)
  {
    auto           arr = contents_[s->str];
    std::vector<T> elements;
    elements.resize(arr.size());

    try
    {
      for (std::size_t i = 0; i < arr.size(); ++i)
      {
        elements[i] = arr[i].As<T>();
      }
    }
    catch (std::runtime_error const &e)
    {
      vm_->RuntimeError(e.what());
    }
    return CreateNewPrimitiveArray(this->vm_, elements);
  }

  template <typename T>
  void SetPrimitive(vm::Ptr<vm::String> const &s, T value)
  {
    contents_[s->str] = value;
  }

  template <typename T>
  void SetArray(vm::Ptr<vm::String> const &s, vm::Ptr<vm::Array<T>> const &arr)
  {
    auto variant_array = variant::Variant::Array(arr->elements.size());

    for (std::size_t i = 0; i < arr->elements.size(); ++i)
    {
      variant_array[i] = arr->elements[i];
    }

    contents_[s->str] = variant_array;
  }

  void SetString(vm::Ptr<vm::String> const &s, vm::Ptr<vm::String> const &value)
  {
    contents_[s->str] = byte_array::ToBase64(value->str);
  }

  void SetBuffer(vm::Ptr<vm::String> const &s, vm::Ptr<ByteArrayWrapper> const &value)
  {
    contents_[s->str] = byte_array::ToBase64(value->byte_array());
  }

  Node ToDAGNode()
  {
    std::stringstream body;
    body << contents_.root();

    node_.type     = Node::DATA;
    node_.contents = body.str();

    return node_;
  }

private:
  Node               node_;
  json::JSONDocument contents_{};
};

}  // namespace vm_modules
}  // namespace fetch
