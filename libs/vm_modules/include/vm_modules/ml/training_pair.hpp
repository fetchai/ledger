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

#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"

#include <utility>

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace ml {

class VMTrainingPair : public fetch::vm::Object,
                       public std::pair<fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>,
                                        fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>>
{
public:
  VMTrainingPair(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                 fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ta,
                 fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> tb);

  static void Bind(vm::Module &module);

  static fetch::vm::Ptr<VMTrainingPair> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ta,
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> tb);

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> data() const;

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> label() const;
};

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
