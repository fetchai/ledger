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

#include "vm_modules/ml/utilities/mnist_utilities.hpp"

namespace fetch {
namespace vm_modules {
namespace ml {
namespace utilities {

vm::Ptr<math::VMTensor> load_mnist_images(vm::VM *vm, vm::Ptr<vm::String> const &filename)
{
  math::VMTensor::TensorType tensor =
      fetch::ml::utilities::read_mnist_images<math::VMTensor::TensorType>(filename->string());
  return vm->CreateNewObject<math::VMTensor>(tensor);
}

vm::Ptr<math::VMTensor> load_mnist_labels(vm::VM *vm, vm::Ptr<vm::String> const &filename)
{
  math::VMTensor::TensorType tensor =
      fetch::ml::utilities::read_mnist_labels<math::VMTensor::TensorType>(filename->string());
  tensor = fetch::ml::utilities::convert_labels_to_onehot(tensor);

  return vm->CreateNewObject<math::VMTensor>(tensor);
}

void BindMNISTUtils(vm::Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateFreeFunction("loadMNISTImages", &load_mnist_images, vm::MAXIMUM_CHARGE);
    module.CreateFreeFunction("loadMNISTLabels", &load_mnist_labels, vm::MAXIMUM_CHARGE);
  }
}

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
