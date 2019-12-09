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

#include "vm_modules/math/utilities/factory_with_estimator.hpp"
#include "vm_modules/ml/utilities/factory_with_estimator.hpp"

namespace vm_modules {
namespace ml {
namespace factory_with_estimator {

namespace maf = math::factory_with_estimator;

//////////////////////////////
// VM model related helpers //
//////////////////////////////

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(VMPtr &vm)
{
  auto model_category = maf::vmString(vm, "sequential");
  auto model          = vm->CreateNewObject<fetch::vm_modules::ml::model::VMModel>(model_category);
  return model;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations)
{
  if (sizes.size() != (activations.size() + 1))
  {
    throw std::runtime_error{"Wrong configuration for multilayer VMModel"};
  }

  auto model           = vmSequentialModel(vm);
  auto size            = activations.size();
  auto layer_type      = maf::vmString(vm, "dense");
  auto activation_type = maf::vmString(vm, "relu");

  for (std::size_t i{0}; i < size; ++i)
  {
    auto input_size  = sizes[i];
    auto output_size = sizes[i + 1];

    if (activations[i])
    {
      model->Estimator().LayerAddDenseActivation(layer_type, input_size, output_size,
                                                 activation_type);
      model->LayerAddDenseActivation(layer_type, input_size, output_size, activation_type);
    }
    else
    {
      model->Estimator().LayerAddDense(layer_type, input_size, output_size);
      model->LayerAddDense(layer_type, input_size, output_size);
    }
  }

  return model;
}

fetch::vm::Ptr<fetch::vm_modules::ml::model::VMModel> vmSequentialModel(
    VMPtr &vm, std::vector<SizeType> &sizes, std::vector<bool> &activations,
    std::string const &loss, std::string const &optimiser)
{
  auto model = vmSequentialModel(vm, sizes, activations);

  // compile model
  auto vm_loss      = maf::vmString(vm, loss);
  auto vm_optimiser = maf::vmString(vm, optimiser);
  model->Estimator().CompileSequential(vm_loss, vm_optimiser);
  model->CompileSequential(vm_loss, vm_optimiser);

  return model;
}
}  // namespace factory_with_estimator
}  // namespace ml
}  // namespace vm_modules
