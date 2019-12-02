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

#include "ml/model/model.hpp"
#include "vm/array.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/tensor.hpp"
#include "vm_modules/math/type.hpp"

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
  CONV2D
};

class VMModel : public fetch::vm::Object
{
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
  using SequentialModelPtr  = std::shared_ptr<fetch::ml::model::Sequential<TensorType>>;

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
          fetch::vm::Ptr<fetch::vm::String> const &model_category);

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &model_category);

  static fetch::vm::Ptr<VMModel> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::String> const &model_category);

  template <typename... LayerArgs>
  void AddLayer(fetch::vm::Ptr<fetch::vm::String> const &layer, LayerArgs... args);

  void CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                         fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  void CompileSimple(fetch::vm::Ptr<fetch::vm::String> const &        optimiser,
                     fetch::vm::Ptr<vm::Array<math::SizeType>> const &layer_shapes);

  void Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
           ::fetch::math::SizeType const &batch_size);

  DataType Evaluate();

  vm::Ptr<VMTensor> Predict(vm::Ptr<VMTensor> const &data);

  static void Bind(fetch::vm::Module &module);

  void SetModel(ModelPtrType const &instance);

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  fetch::vm::Ptr<fetch::vm::String> SerializeToString();

  fetch::vm::Ptr<VMModel> DeserializeFromString(
      fetch::vm::Ptr<fetch::vm::String> const &model_string);

private:
  ModelPtrType       model_;
  ModelConfigPtrType model_config_;
  ModelCategory      model_category_ = ModelCategory::NONE;
  bool               compiled_       = false;

  // First for input layer shape, second for output layer shape.
  static constexpr std::size_t min_total_layer_shapes = 2;

  static const std::map<std::string, SupportedLayerType>                 layer_types_;
  static const std::map<std::string, fetch::ml::details::ActivationType> activations_;
  static const std::map<std::string, fetch::ml::ops::LossType>           losses_;
  static const std::map<std::string, fetch::ml::OptimiserType>           optimisers_;
  static const std::map<std::string, uint8_t>                            model_categories_;

  void Init(std::string const &model_category);

  void PrepareDataloader();

  void AddLayerSpecificImpl(SupportedLayerType layer, math::SizeType const &inputs,
                            math::SizeType const &hidden_nodes);
  void AddLayerSpecificImpl(SupportedLayerType layer, math::SizeType const &inputs,
                            math::SizeType const &                   hidden_nodes,
                            fetch::vm::Ptr<fetch::vm::String> const &activation);
  void AddLayerSpecificImpl(SupportedLayerType layer, math::SizeType const &inputs,
                            math::SizeType const &             hidden_nodes,
                            fetch::ml::details::ActivationType activation);
  void AddLayerSpecificImpl(SupportedLayerType layer, math::SizeType const &output_channels,
                            math::SizeType const &input_channels, math::SizeType const &kernel_size,
                            math::SizeType const &stride_size);
  void AddLayerSpecificImpl(SupportedLayerType layer, math::SizeType const &output_channels,
                            math::SizeType const &input_channels, math::SizeType const &kernel_size,
                            math::SizeType const &                   stride_size,
                            fetch::vm::Ptr<fetch::vm::String> const &activation);
  void AddLayerSpecificImpl(SupportedLayerType layer, math::SizeType const &output_channels,
                            math::SizeType const &input_channels, math::SizeType const &kernel_size,
                            math::SizeType const &             stride_size,
                            fetch::ml::details::ActivationType activation);

  inline void AssertLayerTypeMatches(SupportedLayerType                layer,
                                     std::vector<SupportedLayerType> &&valids) const;

  template <typename T>
  inline T ParseName(std::string const &name, std::map<std::string, T> const &dict,
                     std::string const &errmsg) const;

  inline SequentialModelPtr GetMeAsSequentialIfPossible();
};

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
