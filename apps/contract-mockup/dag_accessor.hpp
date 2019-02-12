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

namespace fetch
{
namespace modules
{

class DAGWrapper : public fetch::vm::Object
{
public:
  DAGWrapper()          = delete;
  virtual ~DAGWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<DAGWrapper>("DAGWrapper")
      .CreateTypeConstuctor<int64_t>()
      .CreateInstanceFunction("next", &DAGWrapper::Next)
      .CreateInstanceFunction("nextAsFloat", &DAGWrapper::NextAsFloat);
  }

  DAGWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, ledger::DAG &dag)
    : fetch::vm::Object(vm, type_id)
    , dag_(dag)
  {}
/*
  static fetch::vm::Ptr<DAGWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             int64_t seed)
  {
    return new VMDAG(vm, type_id, seed);
  }
*/

private:
  ledger::DAG &dag_;
};


}
}