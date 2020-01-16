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

#include "ml/model/model_config.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/serializers/model.hpp"

#include <string>
#include <utility>
#include <vector>

namespace fetch {

namespace vm_modules {
namespace ml {
namespace model {
class ModelEstimator;
}
}  // namespace ml
}  // namespace vm_modules

namespace dmlf {
namespace collective_learning {
template <class TensorType>
class ClientAlgorithm;
}  // namespace collective_learning
}  // namespace dmlf

namespace ml {

namespace dataloaders {
template <typename TensorType>
class DataLoader;
}  // namespace dataloaders

namespace ops {
enum class MetricType;
}  // namespace ops

namespace optimisers {
template <class T>
class Optimiser;
}  // namespace optimisers

namespace model {

template <typename TensorType>
class Model
{
public:
  using DataType           = typename TensorType::Type;
  using SizeType           = fetch::math::SizeType;
  using GraphType          = Graph<TensorType>;
  using DataLoaderType     = dataloaders::DataLoader<TensorType>;
  using ModelOptimiserType = optimisers::Optimiser<TensorType>;
  using GraphPtrType       = typename std::shared_ptr<GraphType>;
  using DataLoaderPtrType  = typename std::shared_ptr<DataLoaderType>;
  using OptimiserPtrType   = typename std::shared_ptr<ModelOptimiserType>;
  using DataVectorType     = std::vector<DataType>;

  explicit Model(ModelConfig<DataType> model_config = ModelConfig<DataType>())
    : model_config_(std::move(model_config))
  {}

  Model(Model const &other);

  virtual ~Model() = default;

  void Compile(OptimiserType optimiser_type, ops::LossType loss_type = ops::LossType::NONE,
               std::vector<ops::MetricType> const &metrics = std::vector<ops::MetricType>());

  /// training and testing ///
  void           Train();
  void           Train(SizeType n_rounds);
  void           Train(SizeType n_rounds, DataType &loss);
  void           Test(DataType &test_loss);
  void           Predict(TensorType &input, TensorType &output);
  DataVectorType Evaluate(dataloaders::DataLoaderMode dl_mode = dataloaders::DataLoaderMode::TEST,
                          SizeType                    batch_size = 0);

  template <typename... Params>
  void SetData(Params... params);

  void UpdateConfig(ModelConfig<DataType> &model_config);

  /// getters and setters ///
  void SetDataloader(std::shared_ptr<DataLoaderType> dataloader_ptr);

  std::shared_ptr<const DataLoaderType>     GetDataloader();
  std::shared_ptr<const ModelOptimiserType> GetOptimiser();

  std::string InputName();
  std::string LabelName();
  std::string OutputName();
  std::string ErrorName();

  bool DataLoaderIsSet();

  template <typename X, typename D>
  friend struct serializers::MapSerializer;
  friend class fetch::vm_modules::ml::model::ModelEstimator;

protected:
  ModelConfig<DataType> model_config_;
  GraphPtrType          graph_ptr_ = std::make_shared<GraphType>();
  DataLoaderPtrType     dataloader_ptr_;
  OptimiserPtrType      optimiser_ptr_;

  std::string              input_;
  std::string              label_;
  std::string              output_;
  std::string              error_;
  std::vector<std::string> metrics_;

  bool loss_set_      = false;
  bool optimiser_set_ = false;
  bool compiled_      = false;

  DataType loss_;

  virtual void PrintStats(SizeType epoch, DataType loss,
                          DataType test_loss = fetch::math::numeric_max<DataType>());

private:
  friend class dmlf::collective_learning::ClientAlgorithm<TensorType>;

  void TrainImplementation(DataType &loss, SizeType n_rounds = 1);
};

template <typename TensorType>
template <typename... Params>
void Model<TensorType>::SetData(Params... params)
{
  dataloader_ptr_->AddData(params...);
}

}  // namespace model
}  // namespace ml
}  // namespace fetch
