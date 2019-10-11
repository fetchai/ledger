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

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "vm/module.hpp"
#include "vm_modules/ml/model/sequential_model.hpp"
#include "vm_modules/ml/state_dict.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {

using SizeType    = fetch::math::SizeType;
using VMPtrString = Ptr<String>;

VMSequentialModel::VMSequentialModel(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  model_config_ = std::make_shared<ModelConfigType>();
  model_        = std::make_shared<fetch::ml::model::Sequential<TensorType>>(*model_config_);
}

Ptr<VMSequentialModel> VMSequentialModel::Constructor(VM *vm, TypeId type_id)
{
  return Ptr<VMSequentialModel>{new VMSequentialModel(vm, type_id)};
}

void VMSequentialModel::LayerAdd(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                 math::SizeType const &inputs, math::SizeType const &hidden_nodes)
{
  LayerAddImplementation(layer->str, inputs, hidden_nodes,
                         fetch::ml::details::ActivationType::NOTHING);
}

void VMSequentialModel::LayerAddActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                           math::SizeType const &                   inputs,
                                           math::SizeType const &                   hidden_nodes,
                                           fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  fetch::ml::details::ActivationType activation_type = fetch::ml::details::ActivationType::NOTHING;
  if (activation->str == "relu")
  {
    activation_type = fetch::ml::details::ActivationType::RELU;
  }
  else
  {
    throw std::runtime_error("attempted to add unknown layer with unknown activation type");
  }
  LayerAddImplementation(layer->str, inputs, hidden_nodes, activation_type);
}

void VMSequentialModel::LayerAddImplementation(std::string const &   layer,
                                               math::SizeType const &inputs,
                                               math::SizeType const &hidden_nodes,
                                               fetch::ml::details::ActivationType const &activation)
{
  // dense / fully connected layer
  if (layer == "dense")
  {
    model_->template Add<fetch::ml::layers::FullyConnected<TensorType>>(inputs, hidden_nodes,
                                                                        activation);
  }
  else
  {
    throw std::runtime_error("attempted to add unknown layer type to sequential model");
  }
}
void VMSequentialModel::Compile(fetch::vm::Ptr<fetch::vm::String> const &loss,
                                fetch::vm::Ptr<fetch::vm::String> const &optimiser)
{
  fetch::ml::ops::LossType loss_type;
  fetch::ml::OptimiserType optimiser_type;

  if (loss->str == "mse")
  {
    loss_type = fetch::ml::ops::LossType::MEAN_SQUARE_ERROR;
  }
  else
  {
    throw std::runtime_error("invalid loss function");
  }

  // dense / fully connected layer
  if (optimiser->str == "adam")
  {
    optimiser_type = fetch::ml::OptimiserType::ADAM;
  }
  else
  {
    throw std::runtime_error("invalid optimiser");
  }

  model_->Compile(optimiser_type, loss_type);
}

void VMSequentialModel::Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
                            fetch::math::SizeType const &batch_size)
{
  // prepare dataloader
  dl_ = std::make_unique<TensorDataloader>();
  dl_->SetRandomMode(true);
  dl_->AddData(data->GetTensor(), labels->GetTensor());
  model_->SetDataloader(std::move(dl_));

  // set batch size
  model_config_->batch_size = batch_size;
  model_->UpdateConfig(*model_config_);

  // train for one epoch
  model_->Train();
}

typename VMSequentialModel::DataType VMSequentialModel::Evaluate()
{
  return model_->Evaluate();
}

void VMSequentialModel::Bind(Module &module)
{
  module.CreateClassType<VMSequentialModel>("SequentialModel")
      .CreateConstructor(&VMSequentialModel::Constructor)
      //      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) ->
      //      Ptr<VMSequentialModel> {
      //        return Ptr<VMSequentialModel>{new VMSequentialModel(vm, type_id)};
      //      })
      .CreateMemberFunction("add", &VMSequentialModel::LayerAdd)
      .CreateMemberFunction("add", &VMSequentialModel::LayerAddActivation)
      .CreateMemberFunction("compile", &VMSequentialModel::Compile)
      .CreateMemberFunction("fit", &VMSequentialModel::Fit)
      .CreateMemberFunction("evaluate", &VMSequentialModel::Evaluate);
}

}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
