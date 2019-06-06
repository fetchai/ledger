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

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "core/json/document.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/dag/dag.hpp"
#include "vm_modules/ledger/chain_state.hpp"
#include "vm_modules/ledger/dag_node_wrapper.hpp"
namespace fetch {
namespace vm_modules {

template <typename T>
vm::Ptr<vm::Array<vm::Ptr<T>>> CreateNewArray(vm::VM * vm, std::vector<vm::Ptr<T>> items)
{
  vm::Ptr<vm::Array<fetch::vm::Ptr<T>>> array =
      new vm::Array<fetch::vm::Ptr<T>>(vm, vm->GetTypeId<vm::IArray>(), vm->GetTypeId<T>(), int32_t(items.size()));
  std::size_t idx = 0;
  for (auto const &e : items)
  {
    array->elements[idx++] = e;
  }
  return array;
  return {};
}

class DAGWrapper : public fetch::vm::Object
{
public:
  DAGWrapper()          = delete;
  virtual ~DAGWrapper() = default;

  static void Bind(vm::Module & module)
  {
    auto interface = module.CreateClassType<DAGWrapper>("DAG");
    interface.CreateConstuctor<>();
    interface.CreateMemberFunction("getNodes", &DAGWrapper::GetNodes);
  }

  DAGWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, ChainState *chain_state)
    : fetch::vm::Object(vm, type_id)
    , chain_state_(chain_state)
    , vm_(vm)
  {}

  static fetch::vm::Ptr<DAGWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    FETCH_UNUSED(vm);
    FETCH_UNUSED(type_id);
//     auto dag = vm->GetGlobalPointer<ChainState>();
//     return new DAGWrapper(vm, type_id, dag);
    return {};
  }

  vm::Ptr<vm::Array<vm::Ptr<DAGNodeWrapper>>> GetNodes()
  {
    std::vector<vm::Ptr<DAGNodeWrapper>> items;

    if (chain_state_->dag == nullptr)
    {
      RuntimeError("DAG pointer is null.");
      return nullptr;
    }


    // TODO(HUT): this.
    //// TODO: Make it such that multiple block times can be extracted
    //auto nodes = chain_state_->dag->ExtractSegment(chain_state_->block);

    //for (auto &n : nodes)
    //{
    //  // We ignore anything that does not reference the past
    //  if (n.previous.size() == 0)
    //  {
    //    continue;
    //  }

    //  // Only data is available inside the contract
    //  if (n.type == ledger::DAGNode::DATA)
    //  {
    //    items.push_back(vm_->CreateNewObject<DAGNodeWrapper>(n));
    //  }
    //}

    return CreateNewArray(vm_, items);
  }

private:
  ChainState *   chain_state_;
  fetch::vm::VM *vm_;
};

}  // namespace vm_modules
}  // namespace fetch
