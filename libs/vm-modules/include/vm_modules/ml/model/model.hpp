#pragma once
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

#include "ml/model/model.hpp"
#include "vm/array.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/type.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"

namespace fetch {

namespace vm {
class Module;
}

namespace ml {
namespace model {

template <typename TensorType>
class Sequential;

}  // namespace model
}  // namespace ml

namespace vm_modules {
namespace ml {
namespace model {

enum class ModelCategory : uint8_t
{
  NONE,
  SEQUENTIAL,
  REGRESSOR,
  CLASSIFIER
};

enum class SupportedLayerType : uint8_t
{
  DENSE,
  CONV1D,
  CONV2D,
  FLATTEN,
  DROPOUT,
  ACTIVATION,
  RESHAPE
};

class VMModel : public fetch::vm::Object
{
  friend class fetch::vm_modules::ml::model::ModelEstimator;

public:
  using DataType            = fetch::vm_modules::math::DataType;
  using TensorType          = fetch::math::Tensor<DataType>;
  using ModelType           = fetch::ml::model::Model<TensorType>;
  using ModelPtrType        = std::shared_ptr<ModelType>;
  using ModelConfigType     = fetch::ml::model::ModelConfig<DataType>;
  using ModelConfigPtrType  = std::shared_ptr<fetch::ml::model::ModelConfig<DataType>>;
  using GraphType           = fetch::ml::Graph<TensorType>;
  using TensorDataloader    = fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>;
  using TensorDataloaderPtr = std::shared_ptr<TensorDataloader>;
  using VMTensor            = fetch::vm_modules::math::VMTensor;
  using ModelEstimator      = fetch::vm_modules::ml::model::ModelEstimator;
  using SequentialModelPtr  = std::shared_ptr<fetch::ml::model::Sequential<TensorType>>;

  VMModel(VMModel const &other) = delete;
  VMModel(VMModel &&other)      = delete;
  VMModel &operator=(VMModel const &other) = default;  // TODO(): Needed for DeserializeFrom
  VMModel &operator=(VMModel &&other) = delete;

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
          fetch::vm::Ptr<fetch::vm::String> const &model_category);

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &model_category);

  static fetch::vm::Ptr<VMModel> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::String> const &model_category);

  void CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                         fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  void CompileSequentialWithMetrics(
      fetch::vm::Ptr<fetch::vm::String> const &                    loss,
      fetch::vm::Ptr<fetch::vm::String> const &                    optimiser,
      fetch::vm::Ptr<vm::Array<vm::Ptr<fetch::vm::String>>> const &metrics);

  void CompileSequentialImplementation(fetch::vm::Ptr<fetch::vm::String> const &      loss,
                                       fetch::vm::Ptr<fetch::vm::String> const &      optimiser,
                                       std::vector<fetch::ml::ops::MetricType> const &metrics);

  void CompileSimple(fetch::vm::Ptr<fetch::vm::String> const &        optimiser,
                     fetch::vm::Ptr<vm::Array<math::SizeType>> const &layer_shapes);

  void Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
           ::fetch::math::SizeType const &batch_size);

  vm::Ptr<vm::Array<math::DataType>> Evaluate();

  vm::Ptr<VMTensor> Predict(vm::Ptr<VMTensor> const &data);

  static void Bind(fetch::vm::Module &module, bool experimental_enabled);

  void SetModel(ModelPtrType const &instance);

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  fetch::vm::Ptr<fetch::vm::String> SerializeToString();

  fetch::vm::Ptr<VMModel> DeserializeFromString(
      fetch::vm::Ptr<fetch::vm::String> const &model_string);

  ModelEstimator &Estimator();

  void LayerAddDense(fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
                     math::SizeType const &hidden_nodes);
  void LayerAddDenseActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                               math::SizeType const &inputs, math::SizeType const &hidden_nodes,
                               fetch::vm::Ptr<fetch::vm::String> const &activation);

  // Experimental Layers
  void LayerAddConv(fetch::vm::Ptr<fetch::vm::String> const &layer,
                    math::SizeType const &output_channels, math::SizeType const &input_channels,
                    math::SizeType const &kernel_size, math::SizeType const &stride_size);
  void LayerAddConvActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                              math::SizeType const &                   output_channels,
                              math::SizeType const &                   input_channels,
                              math::SizeType const &kernel_size, math::SizeType const &stride_size,
                              fetch::vm::Ptr<fetch::vm::String> const &activation);
  void LayerAddDenseActivationExperimental(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                           math::SizeType const &                   inputs,
                                           math::SizeType const &                   hidden_nodes,
                                           fetch::vm::Ptr<fetch::vm::String> const &activation);
  void LayerAddFlatten(fetch::vm::Ptr<fetch::vm::String> const &layer);

  void LayerAddDropout(fetch::vm::Ptr<fetch::vm::String> const &layer,
                       math::DataType const &                   probability);

  void LayerAddActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                          fetch::vm::Ptr<fetch::vm::String> const &activation_name);
  void LayerAddReshape(fetch::vm::Ptr<fetch::vm::String> const &                     layer,
                       fetch::vm::Ptr<fetch::vm::Array<TensorType::SizeType>> const &shape);

private:
  ModelPtrType       model_;
  ModelConfigPtrType model_config_;
  ModelCategory      model_category_ = ModelCategory::NONE;
  ModelEstimator     estimator_;
  bool               compiled_ = false;

  // First for input layer shape, second for output layer shape.
  static constexpr std::size_t min_total_layer_shapes = 2;

  static const std::map<std::string, SupportedLayerType>                 layer_types_;
  static const std::map<std::string, fetch::ml::details::ActivationType> activations_;
  static const std::map<std::string, fetch::ml::ops::LossType>           losses_;
  static const std::map<std::string, fetch::ml::ops::MetricType>         metrics_;
  static const std::map<std::string, fetch::ml::OptimiserType>           optimisers_;
  static const std::map<std::string, uint8_t>                            model_categories_;

  void Init(std::string const &model_category);

  void PrepareDataloader();

  void LayerAddDenseActivationImplementation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                             math::SizeType const &                   inputs,
                                             math::SizeType const &                   hidden_nodes,
                                             fetch::ml::details::ActivationType       activation);

  void LayerAddConvActivationImplementation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                                            math::SizeType const &             output_channels,
                                            math::SizeType const &             input_channels,
                                            math::SizeType const &             kernel_size,
                                            math::SizeType const &             stride_size,
                                            fetch::ml::details::ActivationType activation);

  inline void AssertLayerTypeMatches(SupportedLayerType                layer,
                                     std::vector<SupportedLayerType> &&valids) const;

  template <typename T>
  inline T ParseName(std::string const &name, std::map<std::string, T> const &dict,
                     std::string const &errmsg) const;

  inline SequentialModelPtr GetMeAsSequentialIfPossible();
};

/**
 * Converts between user specified string and output type (e.g. activation, layer etc.)
 * invokes VM runtime error if parsing failed.
 * @param name user specified string to convert
 * @param dict dictionary of existing entities
 * @param errmsg preferred display name of expected type, that was not parsed
 */
template <typename T>
inline T VMModel::ParseName(std::string const &name, std::map<std::string, T> const &dict,
                            std::string const &errmsg) const
{
  if (dict.find(name) == dict.end())
  {
    throw std::runtime_error("Unknown " + errmsg + " name : " + name);
  }
  return dict.at(name);
}

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
