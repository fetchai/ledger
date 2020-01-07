//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "vm/module.hpp"
#include "vm_modules/ml/training_pair.hpp"

#include <utility>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

VMTrainingPair::VMTrainingPair(
    VM *vm, TypeId type_id, Ptr<fetch::vm_modules::math::VMTensor> ta,
    fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>> tb)
  : Object(vm, type_id)
{
  this->first  = std::move(ta);
  this->second = std::move(tb);
}

void VMTrainingPair::Bind(Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateClassType<fetch::vm_modules::ml::VMTrainingPair>("TrainingPair")
        .CreateConstructor(&VMTrainingPair::Constructor, vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("data", &fetch::vm_modules::ml::VMTrainingPair::data,
                              vm::MAXIMUM_CHARGE)
        .CreateMemberFunction("label", &fetch::vm_modules::ml::VMTrainingPair::label,
                              vm::MAXIMUM_CHARGE);
  }
}

Ptr<VMTrainingPair> VMTrainingPair::Constructor(
    VM *vm, TypeId type_id, Ptr<fetch::vm_modules::math::VMTensor> const &ta,
    fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>> const &tb)
{
  return Ptr<VMTrainingPair>{new VMTrainingPair(vm, type_id, ta, tb)};
}

fetch::vm::Ptr<fetch::vm::Array<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>>
VMTrainingPair::data() const
{
  return this->second;
}

Ptr<fetch::vm_modules::math::VMTensor> VMTrainingPair::label() const
{
  return this->first;
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
