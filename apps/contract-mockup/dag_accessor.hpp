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
#include "item.hpp"
namespace fetch
{
namespace modules
{
/*
// 1) register your class and get its interface

auto myclassi = module.CreateClassType<MyClass>("MyClass");

// 2) Register the instantiation:

module.CreateTemplateInstantiationType<fetch::vm::Array, fetch::vm::Ptr<MyClass>>(fetch::vm::TypeIds::IArray);

// 3) setup MyClass

myclassi.CreateInstanceFunction("Test", &MyClass::Test);

void MyClass::Test(fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<MyClass>>> const & array)  { .... }

// Now in the VM  script you can just do:

var a = Array<MyClass>(6);
var mc = MyClass(12, 34.5
  */

  template<typename T>
  vm::Ptr< vm::IArray > CreateNewArray(vm::VM *vm, std::vector< T > items)
  {
    vm::Ptr< vm::Array< fetch::vm::Ptr<T> > > array = new vm::Array< fetch::vm::Ptr<T> > (vm, vm->GetTypeId< vm::IArray >(), int32_t(items.size()));
//    array->elements = std::move(items);
    return array;
  }

class DAGWrapper : public fetch::vm::Object
{
public:
  DAGWrapper()          = delete;
  virtual ~DAGWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<DAGWrapper>("DAG")
      .CreateTypeConstuctor<>()
      .CreateInstanceFunction("getNodes", &DAGWrapper::GetNodes);
  }

  DAGWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, ledger::DAG *dag)
    : fetch::vm::Object(vm, type_id)
    , dag_(dag)
    , vm_(vm)
  { }

  static fetch::vm::Ptr<DAGWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    auto dag = vm->GetGlobalPointer<ledger::DAG> ();
    return new DAGWrapper(vm, type_id, dag);
  }

  
  vm::Ptr< vm::IArray > GetNodes()
  {
    std::vector< ItemWrapper > items;

    FETCH_UNUSED(dag_);

    return CreateNewArray(vm_, items);
  }  
    
private:
  ledger::DAG *dag_{nullptr};
  fetch::vm::VM *vm_;
};


}
}