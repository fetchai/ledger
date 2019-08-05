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
#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/state_dict.hpp"

#include <utility>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using MathTensorType = fetch::math::Tensor<VMStateDict::DataType>;
using VMTensorType   = fetch::vm_modules::math::VMTensor;

VMStateDict::VMStateDict(VM *vm, TypeId type_id)
  : Object(vm, type_id)
  , state_dict_()
{}

VMStateDict::VMStateDict(VM *vm, TypeId type_id, fetch::ml::StateDict<MathTensorType> sd)
  : Object(vm, type_id)
{
  state_dict_ = std::move(sd);
}

Ptr<VMStateDict> VMStateDict::Constructor(VM *vm, TypeId type_id)
{
  return new VMStateDict(vm, type_id);
}

void VMStateDict::SetWeights(Ptr<String> const &nodename, Ptr<math::VMTensor> const &weights)
{
  auto weights_tensor = state_dict_.dict_[nodename->str].weights_;
  *weights_tensor     = weights->GetTensor();
}

void VMStateDict::Bind(Module &module)
{
  module.CreateClassType<VMStateDict>("StateDict")
      .CreateConstructor(&VMStateDict::Constructor)
      .CreateMemberFunction("setWeights", &VMStateDict::SetWeights);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
