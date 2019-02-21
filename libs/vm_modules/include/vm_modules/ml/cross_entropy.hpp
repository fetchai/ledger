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

#include "ml/ops/cross_entropy.hpp"
#include "vm/module.hpp"
#include "vm_modules/ml/tensor.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
class CrossEntropyWrapper : public fetch::vm::Object,
                            public fetch::ml::ops::CrossEntropyLayer<fetch::math::Tensor<float>>
{
public:
  CrossEntropyWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<CrossEntropyWrapper> Constructor(fetch::vm::VM *   vm,
                                                         fetch::vm::TypeId type_id)
  {
    return new CrossEntropyWrapper(vm, type_id);
  }

  float ForwardWrapper(fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &pred,
                       fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &groundTruth)
  {
    return fetch::ml::ops::CrossEntropyLayer<fetch::math::Tensor<float>>::Forward(
        {*pred, *groundTruth});
  }

  fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> BackwardWrapper(
      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &pred,
      fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> const &groundTruth)
  {
    std::shared_ptr<fetch::math::Tensor<float>> dt =
        fetch::ml::ops::CrossEntropyLayer<fetch::math::Tensor<float>>::Backward(
            {*pred, *groundTruth});
    fetch::vm::Ptr<fetch::vm_modules::ml::TensorWrapper> ret =
        this->vm_->CreateNewObject<fetch::vm_modules::ml::TensorWrapper>(dt->shape());
    (*ret)->Copy(*dt);
    return ret;
  }
};

inline void CreateCrossEntropy(std::shared_ptr<fetch::vm::Module> module)
{
  module->CreateClassType<CrossEntropyWrapper>("CrossEntropy")
      .CreateTypeConstuctor<>()
      .CreateInstanceFunction("Forward", &CrossEntropyWrapper::ForwardWrapper)
      .CreateInstanceFunction("Backward", &CrossEntropyWrapper::BackwardWrapper);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
