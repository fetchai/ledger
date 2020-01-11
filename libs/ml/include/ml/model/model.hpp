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

#include "ml/core/graph.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/model/model_config.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/ops/metrics/categorical_accuracy.hpp"
#include "ml/ops/metrics/types.hpp"
#include "ml/optimisation/optimiser.hpp"
#include "ml/optimisation/types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "ml/utilities/graph_saver.hpp"

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

  Model(Model const &other)
  {
    graph_ptr_.reset(other.graph_ptr_.get());
    dataloader_ptr_.reset(other.dataloader_ptr_.get());
    optimiser_ptr_.reset(other.optimiser_ptr_.get());
  }

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

}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for Model
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::model::Model<TensorType>, D>
{
  using Type       = ml::model::Model<TensorType>;
  using DriverType = D;

  // protected member variables
  static uint8_t const GRAPH           = 1;
  static uint8_t const MODEL_CONFIG    = 2;
  static uint8_t const DATALOADER_PTR  = 3;
  static uint8_t const DATALOADER_TYPE = 4;
  static uint8_t const OPTIMISER_PTR   = 5;
  static uint8_t const OPTIMISER_TYPE  = 6;

  static uint8_t const INPUT_NODE_NAME   = 7;
  static uint8_t const LABEL_NODE_NAME   = 8;
  static uint8_t const OUTPUT_NODE_NAME  = 9;
  static uint8_t const ERROR_NODE_NAME   = 10;
  static uint8_t const METRIC_NODE_NAMES = 11;

  static uint8_t const LOSS_SET_FLAG      = 12;
  static uint8_t const OPTIMISER_SET_FLAG = 13;
  static uint8_t const COMPILED_FLAG      = 14;
  static uint8_t const TOTAL_MAP_SIZE     = 14;

  template <typename MapType>
  static void SerializeDataLoader(MapType map, Type const &sp)
  {
    map.Append(DATALOADER_TYPE, static_cast<uint8_t>(sp.dataloader_ptr_->LoaderCode()));

    switch (sp.dataloader_ptr_->LoaderCode())
    {
    case ml::LoaderType::TENSOR:
    {
      auto *loader_ptr = static_cast<fetch::ml::dataloaders::TensorDataLoader<TensorType> *>(
          sp.dataloader_ptr_.get());
      map.Append(DATALOADER_PTR, *loader_ptr);
      break;
    }

    case ml::LoaderType::SGNS:
    case ml::LoaderType::W2V:
    case ml::LoaderType::COMMODITY:
    case ml::LoaderType::C2V:
    {
      throw ml::exceptions::NotImplemented(
          "Serialization for current dataloader type not implemented yet.");
    }

    default:
    {
      throw ml::exceptions::InvalidMode("Unknown dataloader type.");
    }
    }
  }

  template <typename MapType>
  static void SerializeOptimiser(MapType map, Type const &sp)
  {
    map.Append(OPTIMISER_TYPE, static_cast<uint8_t>(sp.optimiser_ptr_->OptimiserCode()));

    switch (sp.optimiser_ptr_->OptimiserCode())
    {
    case ml::OptimiserType::SGD:
    {
      auto *optimiser_ptr =
          static_cast<fetch::ml::optimisers::SGDOptimiser<TensorType> *>(sp.optimiser_ptr_.get());
      map.Append(OPTIMISER_PTR, *optimiser_ptr);
      break;
    }
    case ml::OptimiserType::ADAM:
    {
      auto *optimiser_ptr =
          static_cast<fetch::ml::optimisers::AdamOptimiser<TensorType> *>(sp.optimiser_ptr_.get());
      map.Append(OPTIMISER_PTR, *optimiser_ptr);
      break;
    }
    case ml::OptimiserType::ADAGRAD:
    case ml::OptimiserType::MOMENTUM:
    case ml::OptimiserType::RMSPROP:
    {
      throw ml::exceptions::NotImplemented(
          "serialization for current optimiser type not implemented yet.");
    }

    default:
    {
      throw ml::exceptions::InvalidMode("Unknown optimiser type.");
    }
    }
  }

  template <typename MapType>
  static void DeserializeDataLoader(MapType map, Type &sp)
  {
    uint8_t loader_type{};
    map.ExpectKeyGetValue(DATALOADER_TYPE, loader_type);

    switch (static_cast<ml::LoaderType>(loader_type))
    {
    case ml::LoaderType::TENSOR:
    {
      auto loader_ptr = new ml::dataloaders::TensorDataLoader<TensorType>();
      map.ExpectKeyGetValue(DATALOADER_PTR, *loader_ptr);
      sp.dataloader_ptr_.reset(loader_ptr);
      break;
    }
    case ml::LoaderType::SGNS:
    case ml::LoaderType::W2V:
    case ml::LoaderType::COMMODITY:
    case ml::LoaderType::C2V:
    {
      throw ml::exceptions::NotImplemented(
          "serialization for current dataloader type not implemented yet.");
    }

    default:
    {
      throw ml::exceptions::InvalidMode("Unknown dataloader type.");
    }
    }
  }

  template <typename MapType>
  static void DeserializeOptimiser(MapType map, Type &sp)
  {
    uint8_t optimiser_type{};
    map.ExpectKeyGetValue(OPTIMISER_TYPE, optimiser_type);

    switch (static_cast<ml::OptimiserType>(optimiser_type))
    {
    case ml::OptimiserType::SGD:
    {
      auto optimiser_ptr = new ml::optimisers::SGDOptimiser<TensorType>();
      map.ExpectKeyGetValue(OPTIMISER_PTR, *optimiser_ptr);
      sp.optimiser_ptr_.reset(optimiser_ptr);
      sp.optimiser_ptr_->SetGraph(sp.graph_ptr_);
      sp.optimiser_ptr_->Init();
      break;
    }
    case ml::OptimiserType::ADAM:
    {
      auto optimiser_ptr = new ml::optimisers::AdamOptimiser<TensorType>();
      map.ExpectKeyGetValue(OPTIMISER_PTR, *optimiser_ptr);
      sp.optimiser_ptr_.reset(optimiser_ptr);
      sp.optimiser_ptr_->SetGraph(sp.graph_ptr_);
      sp.optimiser_ptr_->Init();
      break;
    }
    case ml::OptimiserType::ADAGRAD:
    case ml::OptimiserType::MOMENTUM:
    case ml::OptimiserType::RMSPROP:
    {
      throw ml::exceptions::NotImplemented(
          "serialization for current optimiser type not implemented yet.");
    }

    default:
    {
      throw ml::exceptions::InvalidMode("Unknown optimiser type.");
    }
    }
  }

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(TOTAL_MAP_SIZE);

    // serialize the graph first
    map.Append(GRAPH, sp.graph_ptr_->GetGraphSaveableParams());
    map.Append(MODEL_CONFIG, sp.model_config_);

    // serialize dataloader
    SerializeDataLoader(map, sp);

    // serialize optimiser
    SerializeOptimiser(map, sp);

    map.Append(INPUT_NODE_NAME, sp.input_);
    map.Append(LABEL_NODE_NAME, sp.label_);
    map.Append(OUTPUT_NODE_NAME, sp.output_);
    map.Append(ERROR_NODE_NAME, sp.error_);
    map.Append(METRIC_NODE_NAMES, sp.metrics_);

    map.Append(LOSS_SET_FLAG, sp.loss_set_);
    map.Append(OPTIMISER_SET_FLAG, sp.optimiser_set_);
    map.Append(COMPILED_FLAG, sp.compiled_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    // deserialize the graph first
    fetch::ml::GraphSaveableParams<TensorType> gsp;
    map.ExpectKeyGetValue(GRAPH, gsp);
    auto new_graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
    ml::utilities::BuildGraph(gsp, new_graph_ptr);
    sp.graph_ptr_ = new_graph_ptr;

    map.ExpectKeyGetValue(MODEL_CONFIG, sp.model_config_);

    // deserialize dataloader
    DeserializeDataLoader(map, sp);

    // deserialize optimiser
    DeserializeOptimiser(map, sp);

    map.ExpectKeyGetValue(INPUT_NODE_NAME, sp.input_);
    map.ExpectKeyGetValue(LABEL_NODE_NAME, sp.label_);
    map.ExpectKeyGetValue(OUTPUT_NODE_NAME, sp.output_);
    map.ExpectKeyGetValue(ERROR_NODE_NAME, sp.error_);
    map.ExpectKeyGetValue(METRIC_NODE_NAMES, sp.metrics_);

    map.ExpectKeyGetValue(LOSS_SET_FLAG, sp.loss_set_);
    map.ExpectKeyGetValue(OPTIMISER_SET_FLAG, sp.optimiser_set_);
    map.ExpectKeyGetValue(COMPILED_FLAG, sp.compiled_);
  }
};

}  // namespace serializers

}  // namespace fetch
