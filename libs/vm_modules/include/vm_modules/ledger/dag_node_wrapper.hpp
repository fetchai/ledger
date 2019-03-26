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
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "ledger/dag/dag.hpp"
#include "core/json/document.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
namespace fetch
{
namespace vm_modules
{
template<typename T>
vm::Ptr< vm::Array< T > > CreateNewPrimitiveArray(vm::VM *vm, std::vector< T > items)
{
  vm::Ptr< vm::Array< T > > array = new vm::Array< T > (vm, vm->GetTypeId< vm::IArray >(), int32_t(items.size()));
  std::size_t idx = 0;

  for(auto const &e: items)
  {
    array->elements[idx++] = e;
  }

  return array;
}

class DAGNodeWrapper : public fetch::vm::Object
{
public:
  using Node = ledger::DAGNode;

  DAGNodeWrapper()          = delete;
  virtual ~DAGNodeWrapper() = default;

  static void Bind(vm::Module &module)
  {
    auto interface = module.CreateClassType<DAGNodeWrapper>("DAGNode");

    interface
      .CreateTypeConstuctor<>()
      .CreateInstanceFunction("owner", &DAGNodeWrapper::owner)
      .CreateInstanceFunction("has", &DAGNodeWrapper::Has)
      // Getters
      .CreateInstanceFunction("getNumber", &DAGNodeWrapper::GetNumber)      
      .CreateInstanceFunction("getArrayFloat32", &DAGNodeWrapper::GetArray<float>)
      .CreateInstanceFunction("getArrayFloat64", &DAGNodeWrapper::GetArray<double>)      
      .CreateInstanceFunction("getArrayInt32", &DAGNodeWrapper::GetArray<int32_t>)
      .CreateInstanceFunction("getArrayInt64", &DAGNodeWrapper::GetArray<int64_t>)
      .CreateInstanceFunction("getArrayUInt32", &DAGNodeWrapper::GetArray<uint32_t>)
      .CreateInstanceFunction("getArrayUInt64", &DAGNodeWrapper::GetArray<uint64_t>)
      .CreateInstanceFunction("getFloat32", &DAGNodeWrapper::GetPrimitive<float>)
      .CreateInstanceFunction("getFloat64", &DAGNodeWrapper::GetPrimitive<double>)      
      .CreateInstanceFunction("getInt32", &DAGNodeWrapper::GetPrimitive<int32_t>)
      .CreateInstanceFunction("getInt64", &DAGNodeWrapper::GetPrimitive<int64_t>)
      .CreateInstanceFunction("getUInt32", &DAGNodeWrapper::GetPrimitive<uint32_t>)
      .CreateInstanceFunction("getUInt64", &DAGNodeWrapper::GetPrimitive<uint64_t>)
      // Setters
      /*
      .CreateInstanceFunction("setArrayFloat32", &DAGNodeWrapper::SetArray<float>)
      .CreateInstanceFunction("setArrayFloat64", &DAGNodeWrapper::SetArray<double>)      
      .CreateInstanceFunction("setArrayInt32", &DAGNodeWrapper::SetArray<int32_t>)
      .CreateInstanceFunction("setArrayInt64", &DAGNodeWrapper::SetArray<int64_t>)
      .CreateInstanceFunction("setArrayUInt32", &DAGNodeWrapper::SetArray<uint32_t>)
      .CreateInstanceFunction("setArrayUInt64", &DAGNodeWrapper::SetArray<uint64_t>) */
      .CreateInstanceFunction("set", &DAGNodeWrapper::SetPrimitive<float>)
      .CreateInstanceFunction("set", &DAGNodeWrapper::SetPrimitive<double>)      
      .CreateInstanceFunction("set", &DAGNodeWrapper::SetPrimitive<int32_t>)
      .CreateInstanceFunction("set", &DAGNodeWrapper::SetPrimitive<int64_t>)
      .CreateInstanceFunction("set", &DAGNodeWrapper::SetPrimitive<uint32_t>)
      .CreateInstanceFunction("set", &DAGNodeWrapper::SetPrimitive<uint64_t>);      
  }

  DAGNodeWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Node const &node)
    : fetch::vm::Object(vm, type_id)
    , node_(node)
  {
    try {
      contents_.Parse(node_.contents);
    } 
    catch(std::exception const &e)
    {
      vm->RuntimeError(e.what());
    }
  }

  DAGNodeWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {
    // VM can by default only produce DATA
    node_.type = Node::DATA;
    read_only_ = false;
  }  

  static fetch::vm::Ptr<DAGNodeWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new DAGNodeWrapper(vm, type_id);
  }

  fetch::vm::Ptr< ByteArrayWrapper > owner()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(node_.identity.identifier());
  }

  bool Has(vm::Ptr< vm::String > const& s)
  {
    return contents_.Has(s->str);
  }

  double GetNumber(vm::Ptr< vm::String > const& s)
  {
    double ret = 0;
    try 
    {
      ret = contents_[s->str].As<double>();
    }
    catch(std::runtime_error const& e)
    {    
      try 
      {
        ret = static_cast<double>(contents_[s->str].As<int64_t>());
      }
      catch(std::runtime_error const& e)
      {
        vm_->RuntimeError(e.what());
      }
    }
    return ret;
  }

  template< typename T > 
  T GetPrimitive(vm::Ptr< vm::String > const& s)
  {
    T ret = 0;

    try 
    {
      ret = contents_[s->str].As<T>();
    }
    catch(std::runtime_error const& e)
    {
      vm_->RuntimeError(e.what());
    } 
    return ret;
  }


  template< typename T > 
  void SetPrimitive(vm::Ptr< vm::String > const& s, T value)
  {    
    if(read_only_)
    {
      vm_->RuntimeError("DAGNode is read-only.");
    }
    contents_[s->str] = value;
  }

  template< typename T > 
  vm::Ptr< vm::Array< T > > GetArray(vm::Ptr< vm::String > const& s)
  {
    auto arr = contents_[s->str];
    std::vector< T > elements;
    elements.resize(arr.size());

    try
    {
      for(std::size_t i =0; i < arr.size(); ++i)
      {
        elements[i] = arr[i].As< T >();
      }
    }
    catch(std::runtime_error const& e)
    {
      vm_->RuntimeError(e.what());
    }    
    return CreateNewPrimitiveArray(this->vm_, elements);
  }

  Node ToDAGNode() 
  {
    std::stringstream body;
    body << contents_.root();

    node_.type = Node::DATA;    
    node_.contents  = body.str();        

    return node_;
  }
private:
  Node node_;
  json::JSONDocument contents_{};
  bool read_only_{true};
};


}
}