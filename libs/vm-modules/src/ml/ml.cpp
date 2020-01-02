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
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/ml.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/optimisation/optimiser.hpp"
#include "vm_modules/ml/utilities/mnist_utilities.hpp"
#include "vm_modules/ml/utilities/scaler.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

void BindML(Module &module, bool const enable_experimental)
{
  // Tensor - required by later functions
  math::VMTensor::Bind(module, enable_experimental);

  // ml fundamentals
  VMGraph::Bind(module, enable_experimental);

  // dataloader
  VMDataLoader::Bind(module, enable_experimental);

  // optimisers
  VMOptimiser::Bind(module, enable_experimental);

  // model
  model::VMModel::Bind(module, enable_experimental);

  // utilities
  utilities::VMScaler::Bind(module, enable_experimental);
  utilities::BindMNISTUtils(module, enable_experimental);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
