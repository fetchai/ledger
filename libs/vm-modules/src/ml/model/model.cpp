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

#include "core/byte_array/decoders.hpp"
#include "core/serializers/counter.hpp"
#include "ml/charge_estimation/constants.hpp"
#include "ml/charge_estimation/model/constants.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/model/sequential.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/gelu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/metrics/types.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/serializers/ml_types.hpp"
#include "vm/module.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/use_estimator.hpp"

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {
namespace ml {
namespace model {

static constexpr char const *LOGGING_NAME            = "VMModel";
static constexpr char const *NOT_IMPLEMENTED_MESSAGE = " is not yet implemented.";

using fetch::math::SizeType;
using fetch::ml::OptimiserType;
using fetch::ml::details::ActivationType;
using fetch::ml::ops::LossType;
using fetch::ml::ops::MetricType;
using VMPtrString = Ptr<String>;

std::map<std::string, SupportedLayerType> const VMModel::layer_types_{
    {"dense", SupportedLayerType::DENSE},          {"conv1d", SupportedLayerType::CONV1D},
    {"conv2d", SupportedLayerType::CONV2D},        {"flatten", SupportedLayerType::FLATTEN},
    {"dropout", SupportedLayerType::DROPOUT},      {"activation", SupportedLayerType::ACTIVATION},
    {"input", SupportedLayerType::INPUT},          {"reshape", SupportedLayerType::RESHAPE},
    {"maxpool1d", SupportedLayerType::MAXPOOL1D},  {"maxpool2d", SupportedLayerType::MAXPOOL2D},
    {"avgpool1d", SupportedLayerType::AVGPOOL1D},  {"avgpool2d", SupportedLayerType::AVGPOOL2D},
    {"embeddings", SupportedLayerType::EMBEDDINGS}};

std::map<std::string, ActivationType> const VMModel::activations_{
    {"nothing", ActivationType::NOTHING},
    {"leaky_relu", ActivationType::LEAKY_RELU},
    {"log_sigmoid", ActivationType::LOG_SIGMOID},
    {"log_softmax", ActivationType::LOG_SOFTMAX},
    {"relu", ActivationType::RELU},
    {"sigmoid", ActivationType::SIGMOID},
    {"softmax", ActivationType::SOFTMAX},
    {"gelu", ActivationType::GELU},
};

std::map<std::string, LossType> const VMModel::losses_{
    {"mse", LossType::MEAN_SQUARE_ERROR},
    {"cel", LossType::CROSS_ENTROPY},
    {"scel", LossType::SOFTMAX_CROSS_ENTROPY},
};

std::map<std::string, OptimiserType> const VMModel::optimisers_{
    {"adagrad", OptimiserType::ADAGRAD},   {"adam", OptimiserType::ADAM},
    {"momentum", OptimiserType::MOMENTUM}, {"rmsprop", OptimiserType::RMSPROP},
    {"sgd", OptimiserType::SGD},
};

std::map<std::string, uint8_t> const VMModel::model_categories_{
    {"none", static_cast<uint8_t>(ModelCategory::NONE)},
    {"sequential", static_cast<uint8_t>(ModelCategory::SEQUENTIAL)},
};

std::map<std::string, MetricType> const VMModel::metrics_{
    {"categorical accuracy", MetricType::CATEGORICAL_ACCURACY},
    {"cel", MetricType::CROSS_ENTROPY},
    {"mse", MetricType ::MEAN_SQUARE_ERROR},
    {"scel", MetricType ::SOFTMAX_CROSS_ENTROPY},
};

static constexpr SizeType    AUTODETECT_INPUTS      = 0;
static constexpr char const *IMPOSSIBLE_ADD_MESSAGE = "Impossible to add layer : ";
static constexpr char const *LAYER_TYPE_MESSAGE     = "layer type";

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
  uint8_t const parsed_category_num =
      ParseName(model_category, model_categories_, "model category");

  // As far as ParseName succeeded, parsed_category_num is guaranteed to be a valid
  // model category number.
  model_category_ = ModelCategory(parsed_category_num);
  model_config_   = std::make_shared<ModelConfigType>();

  if (model_category_ == ModelCategory::SEQUENTIAL)
  {
    model_ = std::make_shared<fetch::ml::model::Sequential<TensorType>>(*model_config_);
  }
  compiled_ = false;
}

Ptr<VMModel> VMModel::Constructor(VM *vm, TypeId type_id,
                                  fetch::vm::Ptr<fetch::vm::String> const &model_category)
{
  return Ptr<VMModel>{new VMModel(vm, type_id, model_category)};
}
/**
 * @brief VMModel::CompileSequential
 * @param loss a valid loss function ["mse", ...]
 * @param optimiser a valid optimiser name ["adam", "sgd" ...]
 */
void VMModel::CompileSequential(Ptr<String> const &loss, Ptr<String> const &optimiser)
{
  std::vector<MetricType> mets;
  try
  {
    CompileSequentialImplementation(loss, optimiser, mets);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Compile sequential failed: " + std::string(e.what()));
    return;
  }
}

/**
 * @brief VMModel::CompileSequentialWithMetrics
 * @param loss a valid loss function ["mse", ...]
 * @param optimiser a valid optimiser name ["adam", "sgd" ...]
 * @param metrics an array of valid metric names ["categorical accuracy", "mse" ...]
 */
void VMModel::CompileSequentialWithMetrics(Ptr<String> const &loss, Ptr<String> const &optimiser,
                                           Ptr<vm::Array<Ptr<String>>> const &metrics)
{
  try
  {
    // Make vector<MetricType> from vm::Array
    std::size_t const       n_metrics = metrics->elements.size();
    std::vector<MetricType> mets;
    mets.reserve(n_metrics);

    for (std::size_t i = 0; i < n_metrics; ++i)
    {
      Ptr<String> ptr_string = metrics->elements.at(i);
      MetricType  mt         = ParseName(ptr_string->string(), metrics_, "metric");
      mets.emplace_back(mt);
    }
    CompileSequentialImplementation(loss, optimiser, mets);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Compile model failed: " + std::string(e.what()));
    return;
  }
}

void VMModel::CompileSequentialImplementation(Ptr<String> const &loss, Ptr<String> const &optimiser,
                                              std::vector<MetricType> const &metrics)
{
  try
  {
    LossType const      loss_type      = ParseName(loss->string(), losses_, "loss function");
    OptimiserType const optimiser_type = ParseName(optimiser->string(), optimisers_, "optimiser");
    SequentialModelPtr  me             = GetMeAsSequentialIfPossible();
    if (me->LayerCount() == 0)
    {
      vm_->RuntimeError("Cannot compile an empty sequential model, please add layers first.");
      return;
    }
    PrepareDataloader();
    compiled_ = false;
    me->Compile(optimiser_type, loss_type, metrics);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Compilation of a sequential model failed : " + std::string(e.what()));
    return;
  }
  compiled_ = true;
}

void VMModel::Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
                  fetch::math::SizeType const &batch_size)
{
  try
  {
    // set data in the model
    model_->SetData(std::vector<TensorType>{data->GetTensor()}, labels->GetTensor());

    // set batch size
    model_config_->batch_size = batch_size;
    model_->UpdateConfig(*model_config_);

    // train for one epoch
    model_->Train();
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Model fit failed: " + std::string(e.what()));
  }
}

vm::Ptr<Array<math::DataType>> VMModel::Evaluate()
{
  vm::Ptr<Array<math::DataType>> scores;

  try
  {
    auto     ml_scores = model_->Evaluate(fetch::ml::dataloaders::DataLoaderMode::TRAIN);
    SizeType n_scores  = ml_scores.size();

    scores = this->vm_->CreateNewObject<Array<math::DataType>>(
        this->vm_->GetTypeId<math::DataType>(), static_cast<int32_t>(n_scores));

    for (SizeType i{0}; i < n_scores; i++)
    {
      scores->elements.at(i) = ml_scores.at(i);
    }
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Model evaluate failed: " + std::string(e.what()));
  }

  return scores;
}

vm::Ptr<VMModel::VMTensor> VMModel::Predict(vm::Ptr<VMTensor> const &data)
{
  vm::Ptr<VMTensor> prediction = this->vm_->CreateNewObject<VMTensor>(data->shape());
  model_->Predict(data->GetTensor(), prediction->GetTensor());
  return prediction;
}

void VMModel::Bind(Module &module, bool const experimental_enabled)
{
  // model construction always requires initialising some strings, ptrs etc. but is very cheap
  static const ChargeAmount FIXED_CONSTRUCTION_CHARGE{100};

  module.CreateClassType<VMModel>("Model")
      .CreateConstructor(&VMModel::Constructor, FIXED_CONSTRUCTION_CHARGE)
      .CreateSerializeDefaultConstructor([](VM *vm, TypeId type_id) -> Ptr<VMModel> {
        return Ptr<VMModel>{new VMModel(vm, type_id)};
      })
      .CreateMemberFunction("add", &VMModel::LayerAddDense,
                            UseMemberEstimator(&VMModel::EstimateLayerAddDense))
      .CreateMemberFunction("add", &VMModel::LayerAddDenseActivation,
                            UseMemberEstimator(&VMModel::EstimateLayerAddDenseActivation))
      .CreateMemberFunction("compile", &VMModel::CompileSequential,
                            UseMemberEstimator(&VMModel::EstimateCompileSequential))
      .CreateMemberFunction("compile", &VMModel::CompileSequentialWithMetrics,
                            UseMemberEstimator(&VMModel::EstimateCompileSequentialWithMetrics))
      .CreateMemberFunction("fit", &VMModel::Fit, UseMemberEstimator(&VMModel::EstimateFit))
      .CreateMemberFunction("evaluate", &VMModel::Evaluate,
                            UseMemberEstimator(&VMModel::EstimateEvaluate))
      .CreateMemberFunction("predict", &VMModel::Predict,
                            UseMemberEstimator(&VMModel::EstimatePredict))
      .CreateMemberFunction("serializeToString", &VMModel::SerializeToString,
                            UseMemberEstimator(&VMModel::EstimateSerializeToString))
      .CreateMemberFunction("deserializeFromString", &VMModel::DeserializeFromString,
                            UseMemberEstimator(&VMModel::EstimateDeserializeFromString));

  // experimental features are bound only if the VMFactory given the flag to do so
  if (experimental_enabled)
  {
    module.GetClassInterface<VMModel>()
        .CreateMemberFunction("add", &VMModel::LayerAddConv,
                              UseMemberEstimator(&VMModel::EstimateLayerAddConv))
        .CreateMemberFunction("add", &VMModel::LayerAddConvActivation,
                              UseMemberEstimator(&VMModel::EstimateLayerAddConvActivation))
        .CreateMemberFunction("add", &VMModel::LayerAddFlatten,
                              UseMemberEstimator(&VMModel::EstimateLayerAddFlatten))
        .CreateMemberFunction("add", &VMModel::LayerAddDropout,
                              UseMemberEstimator(&VMModel::EstimateLayerAddDropout))
        .CreateMemberFunction("add", &VMModel::LayerAddActivation,
                              UseMemberEstimator(&VMModel::EstimateLayerAddActivation))
        .CreateMemberFunction("add", &VMModel::LayerAddReshape,
                              UseMemberEstimator(&VMModel::EstimateLayerAddReshape))
        .CreateMemberFunction("addExperimental", &VMModel::LayerAddPool,
                              UseMemberEstimator(&VMModel::EstimateLayerAddPool))
        .CreateMemberFunction("addExperimental", &VMModel::LayerAddEmbeddings,
                              UseMemberEstimator(&VMModel::EstimateLayerAddEmbeddings))
        .CreateMemberFunction(
            "addExperimental", &VMModel::LayerAddDenseActivationExperimental,
            UseMemberEstimator(&VMModel::EstimateLayerAddDenseActivationExperimental))
        .CreateMemberFunction("addExperimental", &VMModel::LayerAddInput,
                              UseMemberEstimator(&VMModel::EstimateLayerAddInput))
        .CreateMemberFunction("addExperimental", &VMModel::LayerAddDenseAutoInputs,
                              UseMemberEstimator(&VMModel::EstimateLayerAddDenseAutoInputs));
  }
}

void VMModel::SetModel(const VMModel::ModelPtrType &instance)
{
  model_ = instance;
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
    serializers::SizeCounter counter{};

    counter << static_cast<uint8_t>(model_category_);
    counter << *model_config_;
    counter << compiled_;
    counter << static_cast<uint8_t>(model_->ModelCode());

    if (model_->ModelCode() == fetch::ml::ModelType::SEQUENTIAL)
    {
      auto *model_ptr = static_cast<fetch::ml::model::Sequential<TensorType> *>(model_.get());
      counter << *model_ptr;
    }
    else
    {
      counter << *model_;
    }

    buffer.Reserve(counter.size());

    buffer << static_cast<uint8_t>(model_category_);
    buffer << *model_config_;
    buffer << compiled_;
    buffer << static_cast<uint8_t>(model_->ModelCode());

    if (model_->ModelCode() == fetch::ml::ModelType::SEQUENTIAL)
    {
      auto *model_ptr = static_cast<fetch::ml::model::Sequential<TensorType> *>(model_.get());
      buffer << *model_ptr;
    }
    else
    {
      buffer << *model_;
    }

    success = true;
  }

  return success;
}

bool VMModel::DeserializeFrom(serializers::MsgPackSerializer &buffer)
{
  // deserialise the model category
  uint8_t model_category_int;
  buffer >> model_category_int;

  std::string model_category_name{};
  for (std::pair<std::string, uint8_t> found_category : model_categories_)
  {
    if (found_category.second == model_category_int)
    {
      model_category_name = found_category.first;
    }
  }

  if (model_category_name.empty())
  {
    vm_->RuntimeError("Cannot parse a valid model category from given number : " +
                      std::to_string(model_category_int));
    return false;
  }

  auto const model_category = static_cast<ModelCategory>(model_category_int);

  // deserialise the model config
  ModelConfigType model_config;
  buffer >> model_config;
  model_config_ = std::make_shared<ModelConfigType>(model_config);

  // deserialise the compiled status
  bool compiled = false;
  buffer >> compiled;

  std::shared_ptr<fetch::ml::model::Model<TensorType>> model_ptr;

  uint8_t model_type_int;
  buffer >> model_type_int;
  auto const model_type = static_cast<fetch::ml::ModelType>(model_type_int);

  // deserialise the model
  switch (model_type)
  {
  case fetch::ml::ModelType::SEQUENTIAL:
  {
    auto sequential_ptr = std::make_shared<fetch::ml::model::Sequential<TensorType>>();
    sequential_ptr      = std::make_shared<fetch::ml::model::Sequential<TensorType>>();
    buffer >> (*sequential_ptr);
    model_ptr = sequential_ptr;
    break;
  }
  default:
  {
    model_ptr = std::make_shared<fetch::ml::model::Model<TensorType>>();
    buffer >> (*model_ptr);
    break;
  }
  }

  // assign deserialised model category
  VMModel vm_model(this->vm_, this->type_id_, model_category_name);
  vm_model.model_category_ = model_category;

  // assign deserialised model config
  vm_model.model_config_ = model_config_;

  // assign deserialised model
  vm_model.SetModel(model_ptr);

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

  auto vm_model = fetch::vm::Ptr<VMModel>(new VMModel(vm_, type_id_));
  vm_model->SetModel(model_);

  return vm_model;
}

void VMModel::AssertLayerTypeMatches(SupportedLayerType                layer,
                                     std::vector<SupportedLayerType> &&valids) const
{
  static const std::map<SupportedLayerType, std::string> LAYER_NAMES_{
      {SupportedLayerType::DENSE, "dense"},         {SupportedLayerType::CONV1D, "conv1d"},
      {SupportedLayerType::CONV2D, "conv2d"},       {SupportedLayerType::FLATTEN, "flatten"},
      {SupportedLayerType::DROPOUT, "dropout"},     {SupportedLayerType::ACTIVATION, "activation"},
      {SupportedLayerType::INPUT, "input"},         {SupportedLayerType::MAXPOOL1D, "maxpool1d"},
      {SupportedLayerType::MAXPOOL2D, "maxpool2d"}, {SupportedLayerType::AVGPOOL1D, "avgpool1d"},
      {SupportedLayerType::AVGPOOL2D, "avgpool2d"}, {SupportedLayerType::EMBEDDINGS, "embeddings"}};
  if (std::find(valids.begin(), valids.end(), layer) == valids.end())
  {
    throw std::runtime_error("Invalid params specified for \"" + LAYER_NAMES_.at(layer) +
                             "\" layer.");
  }
}

VMModel::SequentialModelPtr VMModel::GetMeAsSequentialIfPossible()
{
  if (model_category_ != ModelCategory::SEQUENTIAL)
  {
    throw std::runtime_error("Layer adding is allowed only for sequential models!");
  }
  auto sequential_ptr = std::dynamic_pointer_cast<fetch::ml::model::Sequential<TensorType>>(model_);
  if (!sequential_ptr)
  {
    throw std::runtime_error("Cannot cast model pointer to Sequential!");
  }

  return sequential_ptr;
}

void VMModel::LayerAddDense(fetch::vm::Ptr<fetch::vm::String> const &layer,
                            math::SizeType const &inputs, math::SizeType const &hidden_nodes)
{
  LayerAddDenseActivationImplementation(layer, inputs, hidden_nodes, ActivationType::NOTHING);
}

void VMModel::LayerAddDenseAutoInputs(const fetch::vm::Ptr<String> &layer,
                                      const math::SizeType &        hidden_nodes)
{
  LayerAddDenseActivationImplementation(layer, AUTODETECT_INPUTS, hidden_nodes,
                                        ActivationType::NOTHING);
}

void VMModel::LayerAddDenseActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                      math::SizeType const &                   inputs,
                                      math::SizeType const &                   hidden_nodes,
                                      fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  try
  {
    fetch::ml::details::ActivationType activation_type =
        ParseName(activation->string(), activations_, "activation function");

    if (activation_type == fetch::ml::details::ActivationType::RELU)
    {
      LayerAddDenseActivationImplementation(layer, inputs, hidden_nodes, activation_type);
    }
    else
    {
      vm_->RuntimeError("cannot add activation type : " + activation->string());
    }
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }
}

void VMModel::LayerAddDenseActivationExperimental(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
    math::SizeType const &hidden_nodes, fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  try
  {
    fetch::ml::details::ActivationType activation_type =
        ParseName(activation->string(), activations_, "activation function");
    LayerAddDenseActivationImplementation(layer, inputs, hidden_nodes, activation_type);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }
}

void VMModel::LayerAddDenseActivationImplementation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                                    math::SizeType const &                   inputs,
                                                    math::SizeType const &             hidden_nodes,
                                                    fetch::ml::details::ActivationType activation)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::DENSE});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    // if this is a first layer in a Model and inputs count is known,
    // model's InputShape can be set as well.
    if (me->LayerCount() == 0 && inputs != AUTODETECT_INPUTS)
    {
      me->SetBatchInputShape({inputs, 1});
    }
    me->Add<fetch::ml::layers::FullyConnected<TensorType>>(inputs, hidden_nodes, activation);
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddConv(fetch::vm::Ptr<fetch::vm::String> const &layer,
                           math::SizeType const &                   output_channels,
                           math::SizeType const &input_channels, math::SizeType const &kernel_size,
                           math::SizeType const &stride_size)
{
  LayerAddConvActivationImplementation(layer, output_channels, input_channels, kernel_size,
                                       stride_size, ActivationType::NOTHING);
}

void VMModel::LayerAddConvActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                     math::SizeType const &                   output_channels,
                                     math::SizeType const &                   input_channels,
                                     math::SizeType const &                   kernel_size,
                                     math::SizeType const &                   stride_size,
                                     fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  try
  {
    LayerAddConvActivationImplementation(
        layer, output_channels, input_channels, kernel_size, stride_size,
        ParseName(activation->string(), activations_, "activation function"));
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }
}

void VMModel::LayerAddConvActivationImplementation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                                   math::SizeType const &output_channels,
                                                   math::SizeType const &input_channels,
                                                   math::SizeType const &kernel_size,
                                                   math::SizeType const &stride_size,
                                                   fetch::ml::details::ActivationType activation)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::CONV1D, SupportedLayerType::CONV2D});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    if (layer_type == SupportedLayerType::CONV1D)
    {
      me->Add<fetch::ml::layers::Convolution1D<TensorType>>(output_channels, input_channels,
                                                            kernel_size, stride_size, activation);
    }
    else if (layer_type == SupportedLayerType::CONV2D)
    {
      me->Add<fetch::ml::layers::Convolution2D<TensorType>>(output_channels, input_channels,
                                                            kernel_size, stride_size, activation);
    }
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddFlatten(const fetch::vm::Ptr<String> &layer)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::FLATTEN});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    me->Add<fetch::ml::ops::Flatten<TensorType>>();
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddDropout(const fetch::vm::Ptr<String> &layer,
                              const math::DataType &        probability)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::DROPOUT});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();

    // ops::Dropout takes a drop probability
    if (probability < DataType{0} || probability > DataType{1})
    {
      std::stringstream ss;
      ss << IMPOSSIBLE_ADD_MESSAGE << "dropout probability " << probability
         << " is out of allowed range [0..1]";
      vm_->RuntimeError(ss.str());
      return;
    }

    me->Add<fetch::ml::ops::Dropout<TensorType>>(probability);
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddActivation(const fetch::vm::Ptr<String> &layer,
                                 const fetch::vm::Ptr<String> &activation_name)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::ACTIVATION});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();

    ActivationType activation = ParseName(activation_name->string(), activations_, "activation");

    switch (activation)
    {
    case ActivationType::RELU:
      me->Add<fetch::ml::ops::Relu<TensorType>>();
      break;
    case ActivationType::GELU:
      me->Add<fetch::ml::ops::Gelu<TensorType>>();
      break;
    case ActivationType::SIGMOID:
      me->Add<fetch::ml::ops::Sigmoid<TensorType>>();
      break;
    case ActivationType::SOFTMAX:
      me->Add<fetch::ml::ops::Softmax<TensorType>>();
      break;
    case ActivationType::LEAKY_RELU:
      me->Add<fetch::ml::ops::LeakyRelu<TensorType>>();
      break;
    case ActivationType::LOG_SIGMOID:
      me->Add<fetch::ml::ops::LogSigmoid<TensorType>>();
      break;
    case ActivationType::LOG_SOFTMAX:
      me->Add<fetch::ml::ops::LogSoftmax<TensorType>>();
      break;
    default:
      vm_->RuntimeError("Can not add Activation layer with activation type " +
                        activation_name->string());
      return;
    }
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddReshape(const fetch::vm::Ptr<String> &                                layer,
                              const fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> &shape)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::RESHAPE});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    me->Add<fetch::ml::ops::Reshape<TensorType>>(shape->elements);
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

/**
 * @brief Sets expected input shape of the Model; if the shape is not set, hidden layers' shapes
 * can not be automatically deduced/inferred and charge estimation is not possible.
 * @param layer - "input" expected
 * @param shape - input shape, min 2 dimensions, the trailing is batch size.
 */
void VMModel::LayerAddInput(const fetch::vm::Ptr<String> &                   layer,
                            const fetch::vm::Ptr<vm::Array<math::SizeType>> &shape)
{
  if (shape->elements.size() < 2)
  {
    vm_->RuntimeError(
        "Invalid Input layer shape provided: at least 2 dimension needed; the trailing is batch "
        "size.");
    return;
  }
  for (auto const &element : shape->elements)
  {
    if (element == 0)
    {
      vm_->RuntimeError("Invalid Input layer shape provided: dimension with zero size found.");
      return;
    }
  }

  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::INPUT});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    if (me->LayerCount() != 0)
    {
      vm_->RuntimeError(
          "Can not add an Input layer to non-empty Model! Input layer must be first.");
      return;
    }
    me->SetBatchInputShape(shape->elements);
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddPool(const fetch::vm::Ptr<fetch::vm::String> &layer,
                           const math::SizeType &kernel_size, const math::SizeType &stride_size)
{
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type,
                           {SupportedLayerType::MAXPOOL1D, SupportedLayerType::MAXPOOL2D,
                            SupportedLayerType::AVGPOOL1D, SupportedLayerType::AVGPOOL2D});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    if (layer_type == SupportedLayerType::MAXPOOL1D)
    {
      me->Add<fetch::ml::ops::MaxPool1D<TensorType>>(kernel_size, stride_size);
    }
    else if (layer_type == SupportedLayerType::MAXPOOL2D)
    {
      me->Add<fetch::ml::ops::MaxPool2D<TensorType>>(kernel_size, stride_size);
    }
    else if (layer_type == SupportedLayerType::AVGPOOL1D)
    {
      me->Add<fetch::ml::ops::AvgPool1D<TensorType>>(kernel_size, stride_size);
    }
    else if (layer_type == SupportedLayerType::AVGPOOL2D)
    {
      me->Add<fetch::ml::ops::AvgPool2D<TensorType>>(kernel_size, stride_size);
    }
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

void VMModel::LayerAddEmbeddings(const fetch::vm::Ptr<fetch::vm::String> &layer,
                                 const math::SizeType &                   dimensions,
                                 const math::SizeType &data_points, bool stub)
{
  FETCH_UNUSED(stub);  // a neat trick to make a function signature unique
  try
  {
    SupportedLayerType const layer_type =
        ParseName(layer->string(), layer_types_, LAYER_TYPE_MESSAGE);
    AssertLayerTypeMatches(layer_type, {SupportedLayerType::EMBEDDINGS});
    SequentialModelPtr me = GetMeAsSequentialIfPossible();
    me->Add<fetch::ml::ops::Embeddings<TensorType>>(dimensions, data_points);
    compiled_ = false;
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(IMPOSSIBLE_ADD_MESSAGE + std::string(e.what()));
    return;
  }
}

/**
 * for regressor and classifier we can't prepare the dataloder until after compile has begun
 * because model_ isn't ready until then.
 */
void VMModel::PrepareDataloader()
{
  try
  {
    // set up the dataloader
    auto data_loader = std::make_unique<TensorDataloader>();
    data_loader->SetRandomMode(true);
    model_->SetDataloader(std::move(data_loader));
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Can't prepare DataLoader: " + std::string(e.what()));
    return;
  }
}

ChargeAmount VMModel::MaximumCharge(std::string const &log_msg)
{
  FETCH_LOG_ERROR(LOGGING_NAME, "operation charge is vm::MAXIMUM_CHARGE : " + log_msg);
  return vm::MAXIMUM_CHARGE;
}

/**
 * @brief VMModel::EstimatePredict calculates a charge amount, required for a forward pass
 * @param data
 * @return charge estimation
 */
ChargeAmount VMModel::EstimatePredict(const vm::Ptr<math::VMTensor> &data)
{
  ChargeAmount const batch_cost = ChargeForward(data->GetConstTensor().shape());

  FETCH_LOG_INFO(LOGGING_NAME,
                 " forward pass estimated batch cost is " + std::to_string(batch_cost));
  return batch_cost;
}

/**
 * @brief VMModel::EstimateEvaluate calculates a charge amount, required for a forward pass
 * @param data
 * @return charge estimation
 */
ChargeAmount VMModel::EstimateEvaluate()
{
  if (!model_->compiled_)
  {
    throw std::runtime_error("must compile model before evaluating");
  }
  if (!model_->DataLoaderIsSet())
  {
    throw std::runtime_error("must set data before evaluating");
  }

  ChargeAmount const batch_cost = model_->ChargeForward();

  FETCH_LOG_INFO(LOGGING_NAME,
                 " forward pass estimated batch cost is " + std::to_string(batch_cost));
  return batch_cost;
}

ChargeAmount VMModel::EstimateLayerAddDense(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                            math::SizeType const &                   inputs,
                                            math::SizeType const &                   hidden_nodes)
{
  FETCH_UNUSED(layer);

  ChargeAmount charge{0};

  charge = fetch::ml::layers::FullyConnected<TensorType>::ChargeConstruct(inputs, hidden_nodes);

  return charge + 1;
}

ChargeAmount VMModel::EstimateLayerAddDenseActivation(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
    math::SizeType const &hidden_nodes, fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  FETCH_UNUSED(layer);

  ChargeAmount charge{0};
  try
  {
    fetch::ml::details::ActivationType activation_type =
        ParseName(activation->string(), activations_, "activation function");

    if (activation_type == fetch::ml::details::ActivationType::RELU)
    {
      charge = fetch::ml::layers::FullyConnected<TensorType>::ChargeConstruct(inputs, hidden_nodes,
                                                                              activation_type);
    }
    else
    {
      vm_->RuntimeError("cannot add activation type : " + activation->string());
    }
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError(std::string(e.what()));
  }

  return charge + 1;
}

ChargeAmount VMModel::EstimateLayerAddDenseActivationExperimental(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
    math::SizeType const &hidden_nodes, fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  return EstimateLayerAddDenseActivation(layer, inputs, hidden_nodes, activation);
}

ChargeAmount VMModel::EstimateLayerAddDenseAutoInputs(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &hidden_nodes)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(hidden_nodes);
  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

/**
 * @brief VMModel::EstimateCompileSequential
 * @param loss a valid loss function ["mse", ...]
 * @param optimiser a valid optimiser name ["adam", "sgd" ...]
 */
ChargeAmount VMModel::EstimateCompileSequential(Ptr<String> const &loss,
                                                Ptr<String> const &optimiser)
{
  ChargeAmount ret{fetch::ml::charge_estimation::FUNCTION_CALL_COST};

  std::vector<MetricType> mets;
  try
  {
    ret = EstimateCompileSequentialImplementation(loss, optimiser, mets);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Compile sequential failed: " + std::string(e.what()));
    return vm::MAXIMUM_CHARGE;
  }
  return ret;
}

fetch::vm::ChargeAmount VMModel::EstimateCompileSequentialImplementation(
    fetch::vm::Ptr<fetch::vm::String> const &      loss,
    fetch::vm::Ptr<fetch::vm::String> const &      optimiser,
    std::vector<fetch::ml::ops::MetricType> const &metrics)
{
  ChargeAmount op_cnt{fetch::ml::charge_estimation::FUNCTION_CALL_COST};

  try
  {
    LossType const      loss_type      = ParseName(loss->string(), losses_, "loss function");
    OptimiserType const optimiser_type = ParseName(optimiser->string(), optimisers_, "optimiser");
    SequentialModelPtr  me             = GetMeAsSequentialIfPossible();

    // PrepareDataloader(), ;
    op_cnt += fetch::ml::charge_estimation::model::COMPILE_SEQUENTIAL_INIT;

    if (me->LayerCount() == 0)
    {
      vm_->RuntimeError("Can not compile an empty sequential model, please add layers first.");
      return vm::MAXIMUM_CHARGE;
    }

    op_cnt += me->ChargeCompile(optimiser_type, loss_type, metrics);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Compilation of a sequential model failed : " + std::string(e.what()));
    return MAXIMUM_CHARGE;
  }
  // set compiled flag
  op_cnt += fetch::ml::charge_estimation::SET_FLAG;

  return op_cnt;
}

/**
 * @brief VMModel::EstimateCompileSequentialWithMetrics
 * @param loss a valid loss function ["mse", ...]
 * @param optimiser a valid optimiser name ["adam", "sgd" ...]
 * @param metrics an array of valid metric names ["categorical accuracy", "mse" ...]
 */
ChargeAmount VMModel::EstimateCompileSequentialWithMetrics(
    Ptr<String> const &loss, Ptr<String> const &optimiser,
    Ptr<vm::Array<Ptr<String>>> const &metrics)
{
  try
  {
    // Make vector<MetricType> from vm::Array
    std::size_t const       n_metrics = metrics->elements.size();
    std::vector<MetricType> mets;
    mets.reserve(n_metrics);

    for (std::size_t i = 0; i < n_metrics; ++i)
    {
      Ptr<String> ptr_string = metrics->elements.at(i);
      MetricType  mt         = ParseName(ptr_string->string(), metrics_, "metric");
      mets.emplace_back(mt);
    }
    return EstimateCompileSequentialImplementation(loss, optimiser, mets);
  }
  catch (std::exception const &e)
  {
    vm_->RuntimeError("Compile model failed: " + std::string(e.what()));
    return MAXIMUM_CHARGE;
  }
}

ChargeAmount VMModel::EstimateSerializeToString()
{
  auto trainables = model_->graph_ptr_->GetTrainables();

  ChargeAmount estimate{fetch::ml::charge_estimation::FUNCTION_CALL_COST};

  for (auto &w : trainables)
  {
    estimate += TensorType::PaddedSizeFromShape(w->GetWeights().shape());
  }
  return estimate;
}

ChargeAmount VMModel::EstimateDeserializeFromString(Ptr<String> const &model_string)
{
  ChargeAmount estimate = model_string->string().size();

  return estimate + fetch::ml::charge_estimation::FUNCTION_CALL_COST;
}

fetch::vm::ChargeAmount VMModel::EstimateFit(vm::Ptr<math::VMTensor> const &data,
                                             vm::Ptr<math::VMTensor> const &labels,
                                             ::fetch::math::SizeType const &batch_size)
{
  FETCH_UNUSED(labels);

  ChargeAmount estimate{1};

  SizeType subset_size       = data->GetTensor().shape().at(data->GetTensor().shape().size() - 1);
  SizeType number_of_batches = subset_size / batch_size;

  // Forward pass
  estimate += ChargeForward(data->GetTensor().shape()) * number_of_batches;

  // Backward pass
  estimate += ChargeBackward() * subset_size;

  // Optimiser step
  estimate += model_->optimiser_ptr_->ChargeStep() * number_of_batches;

  return estimate;
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddConv(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &output_channels,
    math::SizeType const &input_channels, math::SizeType const &kernel_size,
    math::SizeType const &stride_size)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(output_channels);
  FETCH_UNUSED(input_channels);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddConvActivation(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &output_channels,
    math::SizeType const &input_channels, math::SizeType const &kernel_size,
    math::SizeType const &stride_size, fetch::vm::Ptr<fetch::vm::String> const &activation)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(output_channels);
  FETCH_UNUSED(input_channels);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);
  FETCH_UNUSED(activation);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddFlatten(
    fetch::vm::Ptr<fetch::vm::String> const &layer)
{
  FETCH_UNUSED(layer);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddDropout(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::DataType const &probability)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(probability);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddActivation(
    fetch::vm::Ptr<fetch::vm::String> const &layer,
    fetch::vm::Ptr<fetch::vm::String> const &activation_name)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(activation_name);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddReshape(
    fetch::vm::Ptr<fetch::vm::String> const &                     layer,
    fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &shape)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(shape);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddInput(
    fetch::vm::Ptr<fetch::vm::String> const &        layer,
    fetch::vm::Ptr<vm::Array<math::SizeType>> const &shape)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(shape);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddPool(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &kernel_size,
    math::SizeType const &stride_size)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(kernel_size);
  FETCH_UNUSED(stride_size);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

fetch::vm::ChargeAmount VMModel::EstimateLayerAddEmbeddings(
    fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &dimensions,
    math::SizeType const &data_points, bool stub)
{
  FETCH_UNUSED(layer);
  FETCH_UNUSED(dimensions);
  FETCH_UNUSED(data_points);
  FETCH_UNUSED(stub);

  return MaximumCharge(layer->string() + NOT_IMPLEMENTED_MESSAGE);
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
