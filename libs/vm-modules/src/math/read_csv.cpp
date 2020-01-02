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

#include "math/utilities/ReadCSV.hpp"
#include "vm/module.hpp"
#include "vm_modules/math/read_csv.hpp"
#include "vm_modules/math/tensor/tensor.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace vm_modules {
namespace math {

namespace {

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> ReadCSV(
    fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &filename)
{
  fetch::vm_modules::math::VMTensor::TensorType tensor =
      fetch::math::utilities::ReadCSV<fetch::vm_modules::math::VMTensor::TensorType>(
          filename->string(), 0, 0);
  return vm->CreateNewObject<fetch::vm_modules::math::VMTensor>(tensor);
}

}  // namespace

void BindReadCSV(fetch::vm::Module &module, bool const enable_experimental)
{
  if (enable_experimental)
  {
    module.CreateFreeFunction("readCSV", &ReadCSV);
  }
}

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
