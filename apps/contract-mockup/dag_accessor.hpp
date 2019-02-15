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

#include "core/json/document.hpp"
#include "ledger/dag/dag.hpp"
#include "item.hpp"
namespace fetch
{
namespace modules
{

  template<typename T>
  vm::Ptr< vm::Array< vm::Ptr< T > > > CreateNewArray(vm::VM *vm, std::vector< vm::Ptr< T > > items)
  {
    vm::Ptr< vm::Array< fetch::vm::Ptr<T> > > array = new vm::Array< fetch::vm::Ptr<T> > (vm, vm->GetTypeId< vm::IArray >(), int32_t(items.size()));
    std::size_t idx = 0;

    for(auto const &e: items)
    {
      array->elements[idx++] = e;
    }

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

  vm::Ptr< vm::Array< vm::Ptr< ItemWrapper > > > GetNodes()
  {
    std::vector< vm::Ptr< ItemWrapper > > items;

    if(dag_ == nullptr)
    {
      RuntimeError("DAG pointer is null.");
      return nullptr;
    }

    auto nodes = dag_->nodes();
    for(auto &n : nodes )
    {
      if(n.second.previous.size() == 0)
      {
        continue;
      }

      Item item;
      json::JSONDocument doc; // TODO: Use serialisation
      try {

        doc.Parse(n.second.contents);
      } catch(fetch::json::JSONParseException &e)
      {
        vm_->RuntimeError(e.what());
        return nullptr;
      }

      // TODO: Extend
      item.contract = doc["contract"].As<byte_array::ConstByteArray>();
      item.owner = doc["owner"].As<byte_array::ConstByteArray>();      

      items.push_back( vm_->CreateNewObject< ItemWrapper >( item ) );
    }

    std::cout << "Created list with " << items.size() << " elements" << std::endl;
    return CreateNewArray(vm_, items);
  }  
    
private:
  ledger::DAG *dag_{nullptr};
  fetch::vm::VM *vm_;
};


}
}