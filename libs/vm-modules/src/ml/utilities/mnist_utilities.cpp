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

#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/utilities/mnist_utilities.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {

namespace math {
class VMTensor;
}

namespace ml {
namespace utilities {

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> load_mnist_images(
    fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &filename)
{
  fetch::vm_modules::math::VMTensor::TensorType tensor =
      fetch::ml::utilities::read_mnist_images<fetch::vm_modules::math::VMTensor::TensorType>(
          filename->str);
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(tensor);
}

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> load_mnist_labels(
    fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &filename)
{
  fetch::vm_modules::math::VMTensor::TensorType tensor =
      fetch::ml::utilities::read_mnist_labels<fetch::vm_modules::math::VMTensor::TensorType>(
          filename->str);
  tensor = fetch::ml::utilities::convert_labels_to_onehot(tensor);

  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(tensor);
}

void BindMNISTUtils(fetch::vm::Module &module)
{
  module.CreateFreeFunction("loadMNISTImages", &load_mnist_images);
  module.CreateFreeFunction("loadMNISTLabels", &load_mnist_labels);
}

}  // namespace utilities
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
