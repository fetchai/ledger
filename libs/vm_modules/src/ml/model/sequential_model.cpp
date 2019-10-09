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

//#include "core/byte_array/decoders.hpp"
//#include "ml/core/graph.hpp"
//#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/fully_connected.hpp"
//#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
//#include "ml/saveparams/saveable_params.hpp"
//#include "ml/utilities/graph_builder.hpp"
#include "vm/module.hpp"
//#include "vm_modules/math/tensor.hpp"
//#include "vm_modules/ml/graph.hpp"
#include "vm_modules/ml/state_dict.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using SizeType       = fetch::math::SizeType;
using MathTensorType = fetch::math::Tensor<VMSequentialModel::DataType>;
using VMTensorType   = fetch::vm_modules::math::VMTensor;
using VMPtrString    = Ptr<String>;

VMSequentialModel::VMSequentialModel(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  fetch::ml::model::ModelConfig config{};
  model_ = fetch::ml::model::Model(config);
}

Ptr<VMSequentialModel> VMSequentialModel::Constructor(VM *vm, TypeId type_id)
{
  return Ptr<VMSequentialModel>{new VMSequentialModel(vm, type_id)};
}

void Add(fetch::vm::Ptr<fetch::vm::String> const &layer, fetch::vm::Ptr<fetch::vm::String> const &hidden_nodes)
{
  // dense / fully connected layer
  if (layer == "Dense")
  {
    model.template Add<fetch::ml::layers::FullyConnected<TypeParam>>(hidden_nodes, fetch::ml::details::ActivationType::RELU);
  }
  else
  {
    throw runtime_error("attempted to add unknown layer type to sequential model");
  }
}

void Compile(fetch::vm::Ptr<fetch::vm::String> const &loss, fetch::vm::Ptr<fetch::vm::String> const &optimiser)
{
  fetch::ml::ops::LossType loss_type;
  fetch::ml::OptimiserType optimiser_type;

  if (loss == "MSE")
  {
    loss_type = fetch::ml::ops::LossType::MEAN_SQUARE_ERROR;
  }
  else
  {
    throw runtime_error("invalid loss function");
  }

  // dense / fully connected layer
  if (optimiser == "ADAM")
  {
    optimiser_type = fetch::ml::OptimiserType::ADAM;
  }
  else
  {
    throw runtime_error("invalid loss function");
  }

  model_.compile(optimiser_type, loss_type);
}

void Fit(fetch::vm::Ptr<VMTensor> const &data, fetch::vm::Ptr<VMTensor> const &labels, fetch::math::SizeType batch_size)
{
  // prepare dataloader
  dl_ = TensorDataLoader(data.GetTensor(), labels.GetTensor());
  model_.SetDataLoader(dl);

  // set batch size
  model_config_.batch_size = batch_size;

  // train for one epoch
  model_.Train();
}

void Evaluate()
{
  model_.Evaluate();
}

void VMSequentialModel::Bind(Module &module)
{
  module.CreateClassType<VMSequentialModel>("Graph")
      .CreateConstructor(&VMSequentialModel::Constructor)
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMSequentialModel> {
        return Ptr<VMSequentialModel>{new VMSequentialModel(vm, type_id)};
      })
      .CreateMemberFunction("add", &VMSequentialModel::Add)
      .CreateMemberFunction("compile", &VMSequentialModel::Compile)
      .CreateMemberFunction("fit", &VMSequentialModel::Fit)
      .CreateMemberFunction("evaluate", &VMSequentialModel::Evaluate);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
