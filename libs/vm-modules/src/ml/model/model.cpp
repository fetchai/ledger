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

VMModel::VMModel(VM *vm, TypeId type_id)
  : Object(vm, type_id)
{
  Init("none");
}

VMModel::VMModel(VM *vm, TypeId type_id, fetch::vm::Ptr<fetch::vm::String> const &model_category)
  : Object(vm, type_id)
{
  Init(model_category->string());
}

VMModel::VMModel(VM *vm, TypeId type_id, std::string const &model_category)
  : Object(vm, type_id)
{
  Init(model_category);
}

void VMModel::Init(std::string const &model_category)
{
  model_config_ = std::make_shared<ModelConfigType>();

  if (model_category == "sequential")
  {
    model_          = std::make_shared<fetch::ml::model::Sequential<TensorType>>(*model_config_);
    model_category_ = ModelCategory::SEQUENTIAL;
  }
  else if (model_category == "regressor")
  {
    model_category_ = ModelCategory::REGRESSOR;
  }
  else if (model_category == "classifier")
  {
    model_category_ = ModelCategory::CLASSIFIER;
  }
  else if (model_category == "none")
  {
    model_category_ = ModelCategory::NONE;
  }
  else
  {
    throw std::runtime_error("unknown model type specified.");
  }
  compiled_ = false;
}

Ptr<VMModel> VMModel::Constructor(VM *vm, TypeId type_id,
                                  fetch::vm::Ptr<fetch::vm::String> const &model_category)
{
  return Ptr<VMModel>{new VMModel(vm, type_id, model_category)};
}

void VMModel::LayerAddDense(fetch::vm::Ptr<fetch::vm::String> const &layer,
                            math::SizeType const &inputs, math::SizeType const &hidden_nodes)
{
  // guarantee it's a dense layer
  if (!(layer->string() == "dense"))
  {
    throw std::runtime_error("invalid params specified for " + layer->string() + " layer");
  }

  if (model_category_ == ModelCategory::SEQUENTIAL)
  {
    auto model_ptr = std::dynamic_pointer_cast<fetch::ml::model::Sequential<TensorType>>(model_);
    model_ptr->Add<fetch::ml::layers::FullyConnected<TensorType>>(
        inputs, hidden_nodes, fetch::ml::details::ActivationType::NOTHING);
  }
  else
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }
  compiled_ = false;
}

void VMModel::LayerAddDenseActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                      math::SizeType const &                   inputs,
                                      math::SizeType const &                   hidden_nodes,
                                      fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  // guarantee it's a dense layer
  if (!(layer->string() == "dense"))
  {
    throw std::runtime_error("invalid params specified for " + layer->string() + " layer");
  }

  if (model_category_ == ModelCategory::SEQUENTIAL)
  {
    fetch::ml::details::ActivationType activation_type =
        fetch::ml::details::ActivationType::NOTHING;
    if (activation->string() == "relu")
    {
      activation_type = fetch::ml::details::ActivationType::RELU;
    }
    else
    {
      throw std::runtime_error("attempted to add unknown layer with unknown activation type");
    }
    auto model_ptr = std::dynamic_pointer_cast<fetch::ml::model::Sequential<TensorType>>(model_);
    model_ptr->Add<fetch::ml::layers::FullyConnected<TensorType>>(inputs, hidden_nodes,
                                                                  activation_type);
  }
  else
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }
  compiled_ = false;
}

void VMModel::LayerAddConv(fetch::vm::Ptr<fetch::vm::String> const &layer,
                           math::SizeType const &                   output_channels,
                           math::SizeType const &input_channels, math::SizeType const &kernel_size,
                           math::SizeType const &stride_size)
{
  if (!(model_category_ == ModelCategory::SEQUENTIAL))
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }

  auto model_ptr = std::dynamic_pointer_cast<fetch::ml::model::Sequential<TensorType>>(model_);

  if (layer->string() == "conv1d")
  {
    model_ptr->Add<fetch::ml::layers::Convolution1D<TensorType>>(
        output_channels, input_channels, kernel_size, stride_size,
        fetch::ml::details::ActivationType::NOTHING);
  }
  else if (layer->string() == "conv2d")
  {
    model_ptr->Add<fetch::ml::layers::Convolution2D<TensorType>>(
        output_channels, input_channels, kernel_size, stride_size,
        fetch::ml::details::ActivationType::NOTHING);
  }
  else
  {
    throw std::runtime_error("invalid params specified for " + layer->string() + " layer");
  }
  compiled_ = false;
}

void VMModel::LayerAddConvActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                     math::SizeType const &                   output_channels,
                                     math::SizeType const &                   input_channels,
                                     math::SizeType const &                   kernel_size,
                                     math::SizeType const &                   stride_size,
                                     fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  if (!(model_category_ == ModelCategory::SEQUENTIAL))
  {
    throw std::runtime_error("no add method for non-sequential methods");
  }

  fetch::ml::details::ActivationType activation_type = fetch::ml::details::ActivationType::NOTHING;
  if (activation->string() == "relu")
  {
    activation_type = fetch::ml::details::ActivationType::RELU;
  }
  else
  {
    throw std::runtime_error("attempted to add unknown layer with unknown activation type");
  }

  auto model_ptr = std::dynamic_pointer_cast<fetch::ml::model::Sequential<TensorType>>(model_);
  if (layer->string() == "conv1d")
  {
    model_ptr->Add<fetch::ml::layers::Convolution1D<TensorType>>(
        output_channels, input_channels, kernel_size, stride_size, activation_type);
  }
  else if (layer->string() == "conv2d")
  {
    model_ptr->Add<fetch::ml::layers::Convolution2D<TensorType>>(
        output_channels, input_channels, kernel_size, stride_size, activation_type);
  }
  else
  {
    throw std::runtime_error("invalid params specified for " + layer->string() + " layer");
  }
  compiled_ = false;
}

void VMModel::CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                                fetch::vm::Ptr<fetch::vm::String> const &optimiser)
{
  fetch::ml::ops::LossType loss_type;
  fetch::ml::OptimiserType optimiser_type;

  if (loss->string() == "mse")
  {
    loss_type = fetch::ml::ops::LossType::MEAN_SQUARE_ERROR;
  }
  else if (loss->string() == "cel")
  {
    loss_type = fetch::ml::ops::LossType::CROSS_ENTROPY;
  }
  else if (loss->string() == "scel")
  {
    loss_type = fetch::ml::ops::LossType::SOFTMAX_CROSS_ENTROPY;
  }
  else
  {
    throw std::runtime_error("invalid loss function");
  }

  // dense / fully connected layer
  if (optimiser->string() == "adagrad")
  {
    optimiser_type = fetch::ml::OptimiserType::ADAGRAD;
  }
  else if (optimiser->string() == "adam")
  {
    optimiser_type = fetch::ml::OptimiserType::ADAM;
  }
  else if (optimiser->string() == "momentum")
  {
    optimiser_type = fetch::ml::OptimiserType::MOMENTUM;
  }
  else if (optimiser->string() == "rmsprop")
  {
    optimiser_type = fetch::ml::OptimiserType::RMSPROP;
  }
  else if (optimiser->string() == "sgd")
  {
    optimiser_type = fetch::ml::OptimiserType::SGD;
  }
  else
  {
    throw std::runtime_error("invalid optimiser");
  }

  // Prepare the dataloader
  CompileDataloader();

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

  switch (model_category_)
  {
  case (ModelCategory::REGRESSOR):
  {
    model_ = std::make_shared<fetch::ml::model::DNNRegressor<TensorType>>(*model_config_, layers);
    break;
  }
  case (ModelCategory::CLASSIFIER):
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
  if (optimiser->string() == "adam")
  {
    optimiser_type = fetch::ml::OptimiserType::ADAM;
  }
  else
  {
    throw std::runtime_error("invalid optimiser");
  }

  // Prepare the dataloader
  CompileDataloader();

  model_->Compile(optimiser_type);
}

void VMModel::Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
                  fetch::math::SizeType const &batch_size)
{
  // prepare dataloader
  std::vector<TensorType> train_data{data->GetTensor()};
  model_->SetData(train_data, labels->GetTensor());

  // set batch size
  model_config_->batch_size = batch_size;
  model_->UpdateConfig(*model_config_);

  // train for one epoch
  model_->Train();
}

typename VMModel::DataType VMModel::Evaluate()
{
  return (model_->Evaluate(fetch::ml::dataloaders::DataLoaderMode::TRAIN)).at(0);
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
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMModel> {
        return Ptr<VMModel>{new VMModel(vm, type_id)};
      })
      .CreateMemberFunction("add", &VMModel::LayerAddDense)
      .CreateMemberFunction("add", &VMModel::LayerAddConv)
      .CreateMemberFunction("add", &VMModel::LayerAddDenseActivation)
      .CreateMemberFunction("add", &VMModel::LayerAddConvActivation)
      .CreateMemberFunction("compile", &VMModel::CompileSequential)
      .CreateMemberFunction("compile", &VMModel::CompileSimple)
      .CreateMemberFunction("fit", &VMModel::Fit)
      .CreateMemberFunction("evaluate", &VMModel::Evaluate)
      .CreateMemberFunction("predict", &VMModel::Predict)
      .CreateMemberFunction("evaluate", &VMModel::Evaluate)
      .CreateMemberFunction("predict", &VMModel::Predict)
      .CreateMemberFunction("serializeToString", &VMModel::SerializeToString)
      .CreateMemberFunction("deserializeFromString", &VMModel::DeserializeFromString);
}

typename VMModel::ModelPtrType &VMModel::GetModel()
{
  return model_;
}

bool VMModel::SerializeTo(serializers::MsgPackSerializer &buffer)
{
  bool success = false;

  // can't serialise uncompiled model
  if (!compiled_)
  {
    vm_->RuntimeError("cannot set state with uncompiled model");
  }
  // can't serialise without a model
  else if (!model_)
  {
    vm_->RuntimeError("cannot set state with model undefined");
  }

  // can't serialise without dataloader ready
  else if (!model_->GetDataloader())
  {
    vm_->RuntimeError("cannot set state with dataloader not set");
  }

  // can't serialise without optimiser ready
  else if (!model_->GetOptimiser())
  {
    vm_->RuntimeError("cannot set state with optimiser not set");
  }

  // should be fine to serialise
  else
  {
    buffer << static_cast<uint8_t>(model_category_);
    buffer << *model_config_;
    buffer << compiled_;
    buffer << *model_;
    success = true;
  }

  return success;
}

bool VMModel::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  // deserialise the model category
  uint8_t model_category_int;
  buffer >> model_category_int;
  auto model_category = static_cast<ModelCategory>(model_category_int);

  // deserialise the model config
  ModelConfigType model_config;
  buffer >> model_config;
  model_config_ = std::make_shared<ModelConfigType>(model_config);

  // deserialise the compiled status
  bool compiled;
  buffer >> compiled;

  // deserialise the model
  auto model_ptr = std::make_shared<fetch::ml::model::Model<TensorType>>();
  buffer >> (*model_ptr);

  std::string model_category_str;
  switch (model_category)
  {
  case (ModelCategory::CLASSIFIER):
  {
    model_category_str = "classifier";
    break;
  }
  case (ModelCategory::REGRESSOR):
  {
    model_category_str = "regressor";
    break;
  }
  case (ModelCategory::SEQUENTIAL):
  {
    model_category_str = "sequential";
    break;
  }
  case (ModelCategory::NONE):
  {
    model_category_str = "none";
    break;
  }
  default:
  {
    throw std::runtime_error("cannot deserialise from unspecified model type");
  }
  }

  // assign deserialised model category
  VMModel vm_model(this->vm_, this->type_id_, model_category_str);
  vm_model.model_category_ = model_category;

  // assign deserialised model config
  vm_model.model_config_ = model_config_;

  // assign deserialised model
  vm_model.GetModel() = model_ptr;

  // assign compiled status
  vm_model.compiled_ = compiled;

  // point this object pointer at the deserialised model
  *this = vm_model;

  return true;
}

fetch::vm::Ptr<fetch::vm::String> VMModel::SerializeToString()
{
  serializers::MsgPackSerializer b;
  SerializeTo(b);
  auto byte_array_data = b.data().ToBase64();
  return Ptr<String>{new fetch::vm::String(vm_, static_cast<std::string>(byte_array_data))};
}

fetch::vm::Ptr<VMModel> VMModel::DeserializeFromString(
    fetch::vm::Ptr<fetch::vm::String> const &model_string)
{
  byte_array::ConstByteArray b(model_string->string());
  b = byte_array::FromBase64(b);
  MsgPackSerializer buffer(b);
  DeserializeFrom(buffer);

  auto vm_model        = fetch::vm::Ptr<VMModel>(new VMModel(vm_, type_id_));
  vm_model->GetModel() = model_;

  return vm_model;
}

/**
 * for regressor and classifier we can't prepare the dataloder until after compile has begun
 * because model_ isn't ready until then.
 */
void VMModel::CompileDataloader()
{
  // set up the dataloader
  auto dl = std::make_unique<TensorDataloader>();
  dl->SetRandomMode(true);
  model_->SetDataloader(std::move(dl));

  compiled_ = true;
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
