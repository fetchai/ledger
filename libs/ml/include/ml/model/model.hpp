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

#include "ml/core/graph.hpp"
#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/model/model_config.hpp"
#include "ml/ops/loss_functions/types.hpp"
#include "ml/optimisation/optimiser.hpp"
#include "ml/optimisation/types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "ml/utilities/graph_saver.hpp"

#include <utility>

namespace fetch {
namespace dmlf {
namespace distributed_learning {
template <class TensorType>
class TrainingClient;
}  // namespace distributed_learning
}  // namespace dmlf

namespace ml {
namespace model {

enum class MetricType
{
  LOSS,
  ACCURACY
};

template <typename TensorType>
class Model
{
public:
  using DataType           = typename TensorType::Type;
  using SizeType           = fetch::math::SizeType;
  using GraphType          = Graph<TensorType>;
  using DataLoaderType     = dataloaders::DataLoader<TensorType, TensorType>;
  using ModelOptimiserType = optimisers::Optimiser<TensorType>;
  using GraphPtrType       = typename std::shared_ptr<GraphType>;
  using DataLoaderPtrType  = typename std::shared_ptr<DataLoaderType>;
  using OptimiserPtrType   = typename std::shared_ptr<ModelOptimiserType>;

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

  void Compile(OptimiserType optimiser_type, ops::LossType loss_type = ops::LossType::NONE);
  void SetDataloader(std::shared_ptr<DataLoaderType> dataloader_ptr);
  void SetOptimiser(OptimiserPtrType optimiser_ptr);

  void     Train();
  void     Train(SizeType n_rounds);
  void     Train(SizeType n_rounds, DataType &loss);
  void     Test(DataType &test_loss);
  void     Predict(TensorType &input, TensorType &output);
  DataType Evaluate(std::vector<MetricType> const &metrics = std::vector<MetricType>());

  void UpdateConfig(ModelConfig<DataType> &model_config);

  /// FUNCTIONS THAT EXPOSE THE INTERNALS ///
  std::string InputName();
  std::string LabelName();
  std::string OutputName();
  std::string ErrorName();

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

protected:
  ModelConfig<DataType> model_config_;
  GraphPtrType          graph_ptr_ = std::make_shared<GraphType>();
  DataLoaderPtrType     dataloader_ptr_;
  OptimiserPtrType      optimiser_ptr_;

  std::string input_;
  std::string label_;
  std::string output_;
  std::string error_;

  bool loss_set_      = false;
  bool optimiser_set_ = false;
  bool compiled_      = false;

  DataType loss_;

  virtual void PrintStats(SizeType epoch, DataType loss,
                          DataType test_loss = fetch::math::numeric_max<DataType>());

private:
  friend class dmlf::distributed_learning::TrainingClient<TensorType>;

  bool SetOptimiser();
  void TrainImplementation(DataType &loss, SizeType n_rounds = 1);
};

template <typename TensorType>
void Model<TensorType>::Compile(OptimiserType optimiser_type, ops::LossType loss_type)
{
  // add loss to graph
  if (!loss_set_)
  {
    switch (loss_type)
    {
    case (ops::LossType::CROSS_ENTROPY):
    {
      error_ = graph_ptr_->template AddNode<ops::CrossEntropyLoss<TensorType>>("Error",
                                                                               {output_, label_});
      break;
    }
    case (ops::LossType::MEAN_SQUARE_ERROR):
    {
      error_ = graph_ptr_->template AddNode<ops::MeanSquareErrorLoss<TensorType>>(
          "Error", {output_, label_});
      break;
    }
    case (ops::LossType::SOFTMAX_CROSS_ENTROPY):
    {
      error_ = graph_ptr_->template AddNode<ops::SoftmaxCrossEntropyLoss<TensorType>>(
          "Error", {output_, label_});
      break;
    }
    case (ops::LossType::NONE):
    {
      throw ml::exceptions::InvalidMode(
          "must set loss function on model compile for this model type");
    }
    default:
    {
      throw ml::exceptions::InvalidMode("unrecognised loss type in model compilation");
    }
    }
  }
  else
  {
    if (loss_type != ops::LossType::NONE)
    {
      throw ml::exceptions::InvalidMode(
          "attempted to set loss function on compile but loss function already previously set! "
          "maybe using wrong type of model?");
    }
  }

  // set the optimiser
  if (!optimiser_set_)
  {
    if (!(fetch::ml::optimisers::AddOptimiser<TensorType>(
            optimiser_type, optimiser_ptr_, graph_ptr_, std::vector<std::string>{input_}, label_,
            error_, model_config_.learning_rate_param)))
    {
      throw ml::exceptions::InvalidMode("DNNClassifier initialised with unrecognised optimiser");
    }
    optimiser_set_ = true;
  }

  compiled_ = true;
}

/**
 * Overwrite the models dataloader with an external custom dataloader.
 * @tparam TensorType
 * @param dataloader_ptr
 */
template <typename TensorType>
void Model<TensorType>::SetDataloader(std::shared_ptr<DataLoaderType> dataloader_ptr)
{
  dataloader_ptr_ = dataloader_ptr;
}

/**
 * An interface to train that trains for one epoch
 * @tparam TensorType
 * @return
 */
template <typename TensorType>
void Model<TensorType>::Train()
{
  model_config_.subset_size = fetch::ml::optimisers::SIZE_NOT_SET;
  DataType _;
  Model<TensorType>::TrainImplementation(_);
}

/**
 * An interface to train that doesn't report loss
 * @tparam TensorType
 * @param n_rounds
 * @return
 */
template <typename TensorType>
void Model<TensorType>::Train(SizeType n_rounds)
{
  DataType _;
  Model<TensorType>::Train(n_rounds, _);
}

template <typename TensorType>
void Model<TensorType>::Train(SizeType n_rounds, DataType &loss)
{
  TrainImplementation(loss, n_rounds);
}

template <typename TensorType>
void Model<TensorType>::Test(DataType &test_loss)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before testing");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TEST);

  SizeType test_set_size = dataloader_ptr_->Size();

  dataloader_ptr_->Reset();
  bool is_done_set;
  auto test_pair = dataloader_ptr_->PrepareBatch(test_set_size, is_done_set);

  this->graph_ptr_->SetInput(label_, test_pair.first);
  this->graph_ptr_->SetInput(input_, test_pair.second.at(0));
  test_loss = *(this->graph_ptr_->Evaluate(error_).begin());
}

template <typename TensorType>
void Model<TensorType>::Predict(TensorType &input, TensorType &output)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before predicting");
  }

  this->graph_ptr_->SetInput(input_, input);
  output = this->graph_ptr_->Evaluate(output_);
}

template <typename TensorType>
typename Model<TensorType>::DataType Model<TensorType>::Evaluate(
    std::vector<MetricType> const &metrics)
{
  FETCH_UNUSED(metrics);
  return loss_;
}

template <typename TensorType>
void Model<TensorType>::UpdateConfig(ModelConfig<DataType> &model_config)
{
  model_config_ = model_config;
}

template <typename TensorType>
std::string Model<TensorType>::InputName()
{
  return input_;
}

template <typename TensorType>
std::string Model<TensorType>::LabelName()
{
  return label_;
}

template <typename TensorType>
std::string Model<TensorType>::OutputName()
{
  return output_;
}

template <typename TensorType>
std::string Model<TensorType>::ErrorName()
{
  return error_;
}

template <typename TensorType>
void Model<TensorType>::PrintStats(SizeType epoch, DataType loss, DataType test_loss)
{
  if (this->model_config_.test)
  {
    std::cout << "epoch: " << epoch << ", loss: " << loss << ", test loss: " << test_loss
              << std::endl;
  }
  else
  {
    std::cout << "epoch: " << epoch << ", loss: " << loss << std::endl;
  }
}

template <typename TensorType>
void Model<TensorType>::TrainImplementation(DataType &loss, SizeType n_rounds)
{
  if (!compiled_)
  {
    throw ml::exceptions::InvalidMode("must compile model before training");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TRAIN);

  loss_              = DataType{0};
  DataType min_loss  = fetch::math::numeric_max<DataType>();
  DataType test_loss = fetch::math::numeric_max<DataType>();
  SizeType patience_count{0};
  bool     stop_early = false;

  // run for one subset - if this is not set it defaults to epoch
  loss_ =
      optimiser_ptr_->Run(*dataloader_ptr_, model_config_.batch_size, model_config_.subset_size);
  min_loss = loss_;

  // run for remaining epochs (or subsets) with early stopping
  SizeType step{1};

  while ((!stop_early) && (step < n_rounds))
  {
    if (this->model_config_.print_stats)
    {
      if (this->model_config_.test)
      {
        Test(test_loss);
      }
      PrintStats(step, loss, test_loss);
    }

    if (this->model_config_.save_graph)
    {
      fetch::ml::utilities::SaveGraph(*graph_ptr_,
                                      model_config_.graph_save_location + std::to_string(step));
    }

    // run optimiser for one epoch (or subset)
    loss_ =
        optimiser_ptr_->Run(*dataloader_ptr_, model_config_.batch_size, model_config_.subset_size);

    // update early stopping
    if (this->model_config_.early_stopping)
    {
      if (loss_ < (min_loss - this->model_config_.min_delta))
      {
        min_loss       = loss_;
        patience_count = 0;
      }
      else
      {
        patience_count++;
      }

      if (patience_count >= this->model_config_.patience)
      {
        stop_early = true;
      }
    }

    step++;
  }

  loss = loss_;
}

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

  static uint8_t const INPUT_NODE_NAME  = 7;
  static uint8_t const LABEL_NODE_NAME  = 8;
  static uint8_t const OUTPUT_NODE_NAME = 9;
  static uint8_t const ERROR_NODE_NAME  = 10;

  static uint8_t const LOSS_SET_FLAG      = 11;
  static uint8_t const OPTIMISER_SET_FLAG = 12;
  static uint8_t const COMPILED_FLAG      = 13;

  template <typename MapType>
  static void SerializeDataLoader(MapType map, Type const &sp)
  {
    map.Append(DATALOADER_TYPE, static_cast<uint8_t>(sp.dataloader_ptr_->LoaderCode()));

    switch (sp.dataloader_ptr_->LoaderCode())
    {
    case ml::LoaderType::TENSOR:
    {
      auto *loader_ptr =
          static_cast<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType> *>(
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
    uint8_t loader_type;
    map.ExpectKeyGetValue(DATALOADER_TYPE, loader_type);

    switch (static_cast<ml::LoaderType>(loader_type))
    {
    case ml::LoaderType::TENSOR:
    {
      auto loader_ptr = new ml::dataloaders::TensorDataLoader<TensorType, TensorType>();
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
    uint8_t optimiser_type;
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
    auto map = map_constructor(13);

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

    map.ExpectKeyGetValue(LOSS_SET_FLAG, sp.loss_set_);
    map.ExpectKeyGetValue(OPTIMISER_SET_FLAG, sp.optimiser_set_);
    map.ExpectKeyGetValue(COMPILED_FLAG, sp.compiled_);
  }
};

}  // namespace serializers
}  // namespace fetch
