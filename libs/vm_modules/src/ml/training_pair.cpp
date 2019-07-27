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

#include "vm_modules/ml/training_pair.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {

class VMTrainingPair : public fetch::vm::Object,
                       public std::pair<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>,
                                        fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>
{
public:
  VMTrainingPair(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                 fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ta,
                 fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> tb)
    : fetch::vm::Object(vm, type_id)
  {
    this->first  = ta;
    this->second = tb;
  }

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<fetch::vm_modules::ml::VMTrainingPair>("TrainingPair")
        .CreateConstructor<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>,
                           fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>()
        .CreateMemberFunction("data", &fetch::vm_modules::ml::VMTrainingPair::data)
        .CreateMemberFunction("label", &fetch::vm_modules::ml::VMTrainingPair::label);
  }

  static fetch::vm::Ptr<VMTrainingPair> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ta,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> tb)
  {
    return new VMTrainingPair(vm, type_id, ta, tb);
  }

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> data()
  {
    return this->second;
  }

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> label()
  {
    return this->first;
  }
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
