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
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/model/model.hpp"

#include <memory>
#include <vector>

namespace vm_modules {
namespace ml {
namespace factory_with_estimator {

using VMPtr    = std::shared_ptr<fetch::vm::VM>;
using SizeType = fetch::math::SizeType;

//////////////////////////////
// VM model related helpers //
//////////////////////////////

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(VMPtr &vm);
fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations);
fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations,
    std::string const &loss, std::string const &optimiser);

}  // namespace factory_with_estimator
}  // namespace ml
}  // namespace vm_modules
