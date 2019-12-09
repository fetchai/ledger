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

#include "vm/vm.hpp"
#include "vm_modules/math/tensor/tensor.hpp"

#include <memory>
#include <vector>

namespace vm_modules {
namespace math {
namespace factory_with_estimator {

using VMPtr = std::shared_ptr<fetch::vm::VM>;

////////////////////////
// VM related helpers //
////////////////////////

VMPtr                             NewVM();
fetch::vm::Ptr<fetch::vm::String> vmString(VMPtr &vm, std::string const &str);

/////////////////////////////
// VM math related helpers //
/////////////////////////////

fetch::vm::Ptr<fetch::vm_modules::math::VMTensor> vmTensor(
    VMPtr &vm, std::vector<fetch::math::SizeType> const &shape);

}  // namespace factory_with_estimator
}  // namespace math
}  // namespace vm_modules
