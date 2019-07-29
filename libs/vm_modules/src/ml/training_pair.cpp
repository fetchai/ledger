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

#include "vm/module.hpp"
#include "vm_modules/ml/training_pair.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

VMTrainingPair::VMTrainingPair(VM *vm, TypeId type_id, Ptr<fetch::vm_modules::math::VMTensor> ta,
                               Ptr<fetch::vm_modules::math::VMTensor> tb)
  : Object(vm, type_id)
{
  this->first  = ta;
  this->second = tb;
}

void VMTrainingPair::Bind(Module &module)
{
  module.CreateClassType<fetch::vm_modules::ml::VMTrainingPair>("TrainingPair")
      .CreateConstructor(&VMTrainingPair::Constructor)
      .CreateMemberFunction("data", &fetch::vm_modules::ml::VMTrainingPair::data)
      .CreateMemberFunction("label", &fetch::vm_modules::ml::VMTrainingPair::label);
}

Ptr<VMTrainingPair> VMTrainingPair::Constructor(VM *vm, TypeId type_id,
                                                Ptr<fetch::vm_modules::math::VMTensor> ta,
                                                Ptr<fetch::vm_modules::math::VMTensor> tb)
{
  return new VMTrainingPair(vm, type_id, ta, tb);
}

Ptr<fetch::vm_modules::math::VMTensor> VMTrainingPair::data() const
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
