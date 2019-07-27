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
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/ml.hpp"
#include "vm_modules/ml/optimisation/optimiser.hpp"
#include "vm_modules/ml/state_dict.hpp"
#include "vm_modules/ml/training_pair.hpp"
#include "vm_modules/ml/utilities/scaler.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

void BindML(Module &module)
{
  // Tensor - required by later functions
  math::VMTensor::Bind(module);

  // ml fundamentals
  VMStateDict::Bind(module);
  VMGraph::Bind(module);
  VMTrainingPair::Bind(module);

  // dataloader
  VMDataLoader::Bind(module);

  // optimisers
  VMOptimiser::Bind(module);

  // utilities
  utilities::VMScaler::Bind(module);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
