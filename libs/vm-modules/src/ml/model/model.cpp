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

#include "ml/model/dnn_classifier.hpp"
#include "ml/model/dnn_regressor.hpp"
#include "ml/model/sequential.hpp"

#include "vm/module.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/state_dict.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {
namespace model {

using SizeType    = fetch::math::SizeType;
using VMPtrString = Ptr<String>;

VMModel::VMModel(VM *vm, TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &model_type)
  : Object(vm, type_id)
{
  model_config_ = std::make_shared<ModelConfigType>();

  if (model_type->str == "sequential")
  {
    model_      = std::make_shared<fetch::ml::model::Sequential<TensorType>>(*model_config_);
    model_type_ = fetch::vm_modules::ml::model::ModelType::SEQUENTIAL;
  }
  else if (model_type->str == "regressor")
  {
    model_type_ = fetch::vm_modules::ml::model::ModelType::REGRESSOR;
  }
  else if (model_type->str == "classifier")
  {
    model_type_ = fetch::vm_modules::ml::model::ModelType::CLASSIFIER;
  }
  else
  {
    throw std::runtime_error("unknown model type specified.");
  }
}

Ptr<VMModel> VMModel::Constructor(VM *vm, TypeId type_id,
                                  fetch::vm::Ptr<fetch::vm::String> const &model_type)
{
  return Ptr<VMModel>{new VMModel(vm, type_id, model_type)};
}

void VMModel::LayerAdd(fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
                       math::SizeType const &hidden_nodes)
{
  if (model_type_ == fetch::vm_modules::ml::model::ModelType::SEQUENTIAL)
  {
    LayerAddImplementation(layer->str, inputs, hidden_nodes,
                           fetch::ml::details::ActivationType::NOTHING);
  }
  else
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }
}

void VMModel::LayerAddActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                 math::SizeType const &inputs, math::SizeType const &hidden_nodes,
                                 fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  if (model_type_ == fetch::vm_modules::ml::model::ModelType::SEQUENTIAL)
  {
    fetch::ml::details::ActivationType activation_type =
        fetch::ml::details::ActivationType::NOTHING;
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
  else
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }
}

void VMModel::LayerAddImplementation(std::string const &layer, math::SizeType const &inputs,
                                     math::SizeType const &                    hidden_nodes,
                                     fetch::ml::details::ActivationType const &activation)
{
  if (model_type_ == fetch::vm_modules::ml::model::ModelType::SEQUENTIAL)
  {
    // dense / fully connected layer
    if (layer == "dense")
    {
      auto model_ptr = std::dynamic_pointer_cast<fetch::ml::model::Sequential<TensorType>>(model_);
      model_ptr->Add<fetch::ml::layers::FullyConnected<TensorType>>(inputs, hidden_nodes,
                                                                    activation);
    }
    else
    {
      throw std::runtime_error("attempted to add unknown layer type to sequential model");
    }
  }
  else
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }
}

void VMModel::CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
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

void VMModel::CompileSimple(fetch::vm::Ptr<fetch::vm::String> const &        optimiser,
                            fetch::vm::Ptr<vm::Array<math::SizeType>> const &in_layers)
{
  // construct the model with the specified layers
  auto                        n_elements = in_layers->elements.size();
  std::vector<math::SizeType> layers(n_elements);
  for (std::size_t i = 0; i < n_elements; ++i)
  {
    layers.at(i) = in_layers->elements.at(i);
  }

  switch (model_type_)
  {
  case (vm_modules::ml::model::ModelType::REGRESSOR):
  {
    model_ = std::make_shared<fetch::ml::model::DNNRegressor<TensorType>>(*model_config_, layers);
    break;
  }
  case (vm_modules::ml::model::ModelType::CLASSIFIER):
  {
    model_ = std::make_shared<fetch::ml::model::DNNClassifier<TensorType>>(*model_config_, layers);
    break;
  }
  default:
  {
    throw std::runtime_error("speicified model type does not take layers on compilation");
  }
  }

  // set up the optimiser and compile
  fetch::ml::OptimiserType optimiser_type;
  if (optimiser->str == "adam")
  {
    optimiser_type = fetch::ml::OptimiserType::ADAM;
  }
  else
  {
    throw std::runtime_error("invalid optimiser");
  }

  model_->Compile(optimiser_type);
}
void VMModel::Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
                  fetch::math::SizeType const &batch_size)
{
  // prepare dataloader
  dl_ = std::make_unique<TensorDataloader>();
  dl_->SetRandomMode(true);
  dl_->AddData({data->GetTensor()}, labels->GetTensor());
  model_->SetDataloader(std::move(dl_));

  // set batch size
  model_config_->batch_size = batch_size;
  model_->UpdateConfig(*model_config_);

  // train for one epoch
  model_->Train();
}

typename VMModel::DataType VMModel::Evaluate()
{
  return model_->Evaluate();
}

vm::Ptr<VMModel::VMTensor> VMModel::Predict(vm::Ptr<VMTensor> const &data)
{
  vm::Ptr<VMTensor> prediction = this->vm_->CreateNewObject<VMTensor>(data->shape());
  model_->Predict(data->GetTensor(), prediction->GetTensor());
  return prediction;
}

void VMModel::Bind(Module &module)
{
  module.CreateClassType<VMModel>("Model")
      .CreateConstructor(&VMModel::Constructor)
      .CreateMemberFunction("add", &VMModel::LayerAdd)
      .CreateMemberFunction("add", &VMModel::LayerAddActivation)
      .CreateMemberFunction("compile", &VMModel::CompileSequential)
      .CreateMemberFunction("compile", &VMModel::CompileSimple)
      .CreateMemberFunction("fit", &VMModel::Fit)
      .CreateMemberFunction("evaluate", &VMModel::Evaluate)
      .CreateMemberFunction("predict", &VMModel::Predict);
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
