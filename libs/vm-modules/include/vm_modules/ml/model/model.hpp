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
#include "vm_modules/ml/model/model_estimator.hpp"

namespace fetch {

namespace vm {
class Module;
}

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

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
          fetch::vm::Ptr<fetch::vm::String> const &model_category);

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::string const &model_category);

  static fetch::vm::Ptr<VMModel> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::String> const &model_category);

  void LayerAddDense(fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
                     math::SizeType const &hidden_nodes);
  void LayerAddDenseActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                               math::SizeType const &inputs, math::SizeType const &hidden_nodes,
                               fetch::vm::Ptr<fetch::vm::String> const &activation);

  void LayerAddConv(fetch::vm::Ptr<fetch::vm::String> const &layer,
                    math::SizeType const &output_channels, math::SizeType const &input_channels,
                    math::SizeType const &kernel_size, math::SizeType const &stride_size);
  void LayerAddConvActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                              math::SizeType const &                   output_channels,
                              math::SizeType const &                   input_channels,
                              math::SizeType const &kernel_size, math::SizeType const &stride_size,
                              fetch::vm::Ptr<fetch::vm::String> const &activation);

  void CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                         fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  void CompileSimple(fetch::vm::Ptr<fetch::vm::String> const &        optimiser,
                     fetch::vm::Ptr<vm::Array<math::SizeType>> const &in_layers);

  void Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
           ::fetch::math::SizeType const &batch_size);

  DataType Evaluate();

  vm::Ptr<VMTensor> Predict(vm::Ptr<VMTensor> const &data);

  static void Bind(fetch::vm::Module &module);

  ModelPtrType &GetModel();

  bool SerializeTo(serializers::MsgPackSerializer &buffer) override;

  bool DeserializeFrom(serializers::MsgPackSerializer &buffer) override;

  fetch::vm::Ptr<fetch::vm::String> SerializeToString();

  fetch::vm::Ptr<VMModel> DeserializeFromString(
      fetch::vm::Ptr<fetch::vm::String> const &model_string);

  ModelEstimator &Estimator();

private:
  ModelPtrType       model_;
  ModelConfigPtrType model_config_;
  ModelCategory      model_category_ = ModelCategory::NONE;
  ModelEstimator     estimator_;

  void Init(std::string const &model_category);
};

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
