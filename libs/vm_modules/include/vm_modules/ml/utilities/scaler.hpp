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
#include "vm_modules/math/type.hpp"

#include <memory>

namespace fetch {

namespace ml {
namespace utilities {
template <typename>
class Scaler;
}
}  // namespace ml
namespace vm {
class Module;
}

namespace vm_modules {

namespace math {
class VMTensor;
}

namespace ml {
namespace utilities {

class VMScaler : public fetch::vm::Object
{
public:
  using DataType = fetch::vm_modules::math::DataType;

  VMScaler(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMScaler> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  void SetScale(fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &reference_tensor,
                fetch::vm::Ptr<fetch::vm::String> const &                mode);

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> Normalise(
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &input_tensor);

  fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> DeNormalise(
      fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> const &input_tensor);

  static void Bind(fetch::vm::Module &module);

  std::shared_ptr<fetch::ml::utilities::Scaler<fetch::math::Tensor<DataType>>> scaler_;
};

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
