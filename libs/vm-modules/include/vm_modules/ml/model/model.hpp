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

namespace vm_modules {
namespace ml {
namespace model {

enum class ModelType
{
  NONE,
  SEQUENTIAL,
  REGRESSOR,
  CLASSIFIER
};

class VMModel : public fetch::vm::Object
{
public:
  using DataType            = fetch::vm_modules::math::DataType;
  using TensorType          = fetch::math::Tensor<DataType>;
  using ModelPtrType        = std::shared_ptr<fetch::ml::model::Model<TensorType>>;
  using ModelConfigType     = fetch::ml::model::ModelConfig<DataType>;
  using ModelConfigPtrType  = std::shared_ptr<fetch::ml::model::ModelConfig<DataType>>;
  using GraphType           = fetch::ml::Graph<TensorType>;
  using TensorDataloader    = fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>;
  using TensorDataloaderPtr = std::unique_ptr<TensorDataloader>;
  using VMTensor            = fetch::vm_modules::math::VMTensor;

  VMModel(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
          fetch::vm::Ptr<fetch::vm::String> const &model_type);

  static fetch::vm::Ptr<VMModel> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             fetch::vm::Ptr<fetch::vm::String> const &model_type);

  void LayerAdd(fetch::vm::Ptr<fetch::vm::String> const &layer, math::SizeType const &inputs,
                math::SizeType const &hidden_nodes);
  void LayerAddActivation(fetch::vm::Ptr<fetch::vm::String> const &layer,
                          math::SizeType const &inputs, math::SizeType const &hidden_nodes,
                          fetch::vm::Ptr<fetch::vm::String> const &activation);
  void LayerAddImplementation(std::string const &layer, math::SizeType const &inputs,
                              math::SizeType const &                    hidden_nodes,
                              fetch::ml::details::ActivationType const &activation =
                                  fetch::ml::details::ActivationType::NOTHING);

  void CompileSequential(fetch::vm::Ptr<fetch::vm::String> const &loss,
                         fetch::vm::Ptr<fetch::vm::String> const &optimiser);

  void CompileSimple(fetch::vm::Ptr<fetch::vm::String> const &        optimiser,
                     fetch::vm::Ptr<vm::Array<math::SizeType>> const &in_layers);

  void Fit(vm::Ptr<VMTensor> const &data, vm::Ptr<VMTensor> const &labels,
           fetch::math::SizeType const &batch_size);

  DataType Evaluate();

  vm::Ptr<VMTensor> Predict(vm::Ptr<VMTensor> const &data);

  static void Bind(fetch::vm::Module &module);

private:
  TensorDataloaderPtr dl_;
  ModelPtrType        model_;
  ModelConfigPtrType  model_config_;
  ModelType           model_type_ = ModelType::NONE;
};

}  // namespace model
}  // namespace ml
}  // namespace vm_modules
}  // namespace fetch
