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
#include "byte_array_wrapper.hpp"
namespace fetch
{
namespace modules
{

struct Dummy {
  std::vector< int64_t > shape;
};

class DummyWrapper : public fetch::vm::Object
{
public:
  DummyWrapper()          = delete;
  virtual ~DummyWrapper() = default;

  static void Bind(vm::Module &module)
  {
    auto interface = module.CreateClassType<DummyWrapper>("Dummy");

    interface
      .CreateTypeConstuctor<vm::Ptr< vm::Array<int64_t> > >();

  }

  DummyWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Dummy const &dummy)
    : fetch::vm::Object(vm, type_id)
    , dummy_(dummy)
  {

    std::cout << this->type_id_ << std::endl;    
  }

  static fetch::vm::Ptr<DummyWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, vm::Ptr< vm::Array< int64_t > > arr)
  {

    return new DummyWrapper(vm, type_id, Dummy{{1, 2, 3}});
  }


private:
  Dummy dummy_;
};


}
}