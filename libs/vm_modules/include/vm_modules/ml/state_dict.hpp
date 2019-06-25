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

#include "ml/state_dict.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/tensor.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {

class VMStateDict : public fetch::vm::Object
{
public:
  using DataType       = float;
  using MathTensorType = fetch::math::Tensor<DataType>;
  using VMTensorType   = fetch::vm_modules::math::VMTensor;

  VMStateDict(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
    , state_dict_()
  {}

  VMStateDict(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::ml::StateDict<MathTensorType> sd)
    : fetch::vm::Object(vm, type_id)
  {
    state_dict_ = std::move(sd);
  }

  static fetch::vm::Ptr<VMStateDict> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new VMStateDict(vm, type_id);
  }

  void SetWeights(fetch::vm::Ptr<fetch::vm::String> const &                nodename,
                  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &weights)
  {
    auto weights_tensor = state_dict_.dict_[nodename->str].weights_;
    *weights_tensor     = weights->GetTensor();
  }

  static void Bind(fetch::vm::Module &module)
  {
    module.CreateClassType<VMStateDict>("StateDict")
        .CreateConstuctor<>()
        .CreateMemberFunction("SetWeights", &VMStateDict::SetWeights);
  }

  fetch::ml::StateDict<MathTensorType> state_dict_;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
