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

struct Tensor {
  std::vector< int64_t > shape;
};

class TensorWrapper : public fetch::vm::Object
{
public:
  TensorWrapper()          = delete;
  virtual ~TensorWrapper() = default;

  static void Bind(vm::Module &module)
  {
    auto interface = module.CreateClassType<TensorWrapper>("Tensor");

   module.CreateTemplateInstantiationType<fetch::vm::Array, int64_t >(fetch::vm::TypeIds::IArray);   

    interface
      .CreateTypeConstuctor<vm::Ptr< vm::Array<int64_t> > >()
      .CreateInstanceFunction("shape", &TensorWrapper::shape);
  }

  TensorWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, Tensor const &dummy)
    : fetch::vm::Object(vm, type_id)
    , tensor_(dummy)
  {

  }

  static fetch::vm::Ptr<TensorWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, vm::Ptr< vm::Array< int64_t > > arr)
  {
    return new TensorWrapper(vm, type_id, Tensor{arr->elements});
  }

  vm::Ptr< vm::Array< int64_t > > shape()
  {
    vm::Ptr< vm::Array< int64_t > > ret = new vm::Array< int64_t >(this->vm_, this->vm_->GetTypeId< vm::Array< int64_t > >(), int32_t(tensor_.shape.size()) );
    std::copy(tensor_.shape.begin(), tensor_.shape.end(), ret->elements.begin());

    return ret;
  }
private:
  Tensor tensor_;
};


}
}