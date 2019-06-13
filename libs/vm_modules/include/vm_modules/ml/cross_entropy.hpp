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

#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/tensor.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
class VMCrossEntropyLoss : public fetch::vm::Object,
                            public fetch::ml::ops::CrossEntropy<fetch::math::Tensor<float>>
{
public:
  VMCrossEntropyLoss(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<VMCrossEntropyLoss> Constructor(fetch::vm::VM *   vm,
                                                         fetch::vm::TypeId type_id)
  {
    return new VMCrossEntropyLoss(vm, type_id);
  }

  float ForwardWrapper(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &pred,
                       fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &groundTruth)
  {
    return fetch::ml::ops::CrossEntropy<fetch::math::Tensor<float>>::Forward({*pred, *groundTruth});
  }

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> BackwardWrapper(
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &pred,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &groundTruth)
  {
    fetch::math::Tensor<float> dt =
        fetch::ml::ops::CrossEntropy<fetch::math::Tensor<float>>::Backward({*pred, *groundTruth});
    fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ret =
        this->vm_->CreateNewObject<fetch::vm_modules::math::VMTensor>(dt.shape());
    (*ret).Copy(dt);
    return ret;
  }
};

inline void CreateCrossEntropy(fetch::vm::Module &module)
{
  module.CreateClassType<VMCrossEntropyLoss>("CrossEntropy")
      .CreateConstuctor<>()
      .CreateMemberFunction("Forward", &VMCrossEntropyLoss::ForwardWrapper)
      .CreateMemberFunction("Backward", &VMCrossEntropyLoss::BackwardWrapper);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
