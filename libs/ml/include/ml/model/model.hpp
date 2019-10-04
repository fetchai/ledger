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

#include <utility>

namespace fetch {
namespace ml {
namespace model {

template <typename TensorType>
class Model
{
public:
  using DataType          = typename TensorType::Type;
  using SizeType          = fetch::math::SizeType;
  using GraphType         = Graph<TensorType>;
  using DataLoaderType    = dataloaders::DataLoader<TensorType, TensorType>;
  using GraphPtrType      = typename std::shared_ptr<GraphType>;
  using DataLoaderPtrType = typename std::unique_ptr<DataLoaderType>;
  using OptimiserPtrType  = typename std::unique_ptr<optimisers::Optimiser<TensorType>>;

  explicit Model(ModelConfig<DataType> model_config = ModelConfig<DataType>())
    : model_config_(std::move(model_config))
  {}

  void Compile(OptimiserType optimiser_type, ops::LossType loss_type = ops::LossType::NONE);
  void SetDataloader(std::unique_ptr<DataLoaderType> dataloader_ptr);

  virtual void Train(SizeType n_steps);
  virtual void Train(SizeType n_steps, DataType &loss);
  virtual void Test(DataType &test_loss);
  virtual void Predict(TensorType &input, TensorType &output);

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

protected:
  ModelConfig<DataType> model_config_;
  GraphPtrType          graph_ptr_ = std::make_unique<GraphType>();
  DataLoaderPtrType     dataloader_ptr_;
  OptimiserPtrType      optimiser_ptr_;

  std::string input_;
  std::string label_;
  std::string output_;
  std::string error_;

  bool loss_set_      = false;
  bool optimiser_set_ = false;
  bool compiled_      = false;

  virtual void PrintStats(SizeType epoch, DataType loss,
                          DataType test_loss = fetch::math::numeric_max<DataType>());

private:
  bool SetOptimiser();
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
      throw std::runtime_error("must set loss function on model compile for this model type");
    }
    default:
    {
      throw std::runtime_error("unrecognised loss type in model compilation");
    }
    }
  }
  else
  {
    if (loss_type != ops::LossType::NONE)
    {
      throw std::runtime_error(
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
      throw std::runtime_error("DNNClassifier initialised with unrecognised optimiser");
    }
    optimiser_set_ = true;
  }

  compiled_ = true;
}

/**
 * Overwrite the models dataloader with an external custom dataloader. It must be
 * moved in as a unique_ptr to ensure ownership entirely belongs to model
 * @tparam TensorType
 * @param dataloader_ptr
 */
template <typename TensorType>
void Model<TensorType>::SetDataloader(std::unique_ptr<DataLoaderType> dataloader_ptr)
{
  dataloader_ptr_ = std::move(dataloader_ptr);
}

/**
 * An interface to train that doesn't report loss
 * @tparam TensorType
 * @param n_steps
 * @return
 */
template <typename TensorType>
void Model<TensorType>::Train(SizeType n_steps)
{
  DataType _;
  Model<TensorType>::Train(n_steps, _);
}

template <typename TensorType>
void Model<TensorType>::Train(SizeType n_steps, DataType &loss)
{
  if (!compiled_)
  {
    throw std::runtime_error("must compile model before training");
  }

  dataloader_ptr_->SetMode(dataloaders::DataLoaderMode::TRAIN);

  loss               = DataType{0};
  DataType min_loss  = fetch::math::numeric_max<DataType>();
  DataType test_loss = fetch::math::numeric_max<DataType>();
  SizeType patience_count{0};
  bool     stop_early = false;

  // run for one epoch
  loss = optimiser_ptr_->Run(*dataloader_ptr_, model_config_.batch_size, model_config_.subset_size);
  min_loss = loss;

  // run for remaining epochs with early stopping
  SizeType step{1};
  while ((!stop_early) && (step < n_steps))
  {
    if (this->model_config_.print_stats)
    {
      if (this->model_config_.test)
      {
        Test(test_loss);
      }
      PrintStats(step, loss, test_loss);
    }

    // run optimiser for one epoch
    loss =
        optimiser_ptr_->Run(*dataloader_ptr_, model_config_.batch_size, model_config_.subset_size);

    // update early stopping
    if (this->model_config_.early_stopping)
    {
      if (loss < (min_loss - this->model_config_.min_delta))
      {
        min_loss       = loss;
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
}

template <typename TensorType>
void Model<TensorType>::Test(DataType &test_loss)
{
  if (!compiled_)
  {
    throw std::runtime_error("must compile model before testing");
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
    throw std::runtime_error("must compile model before predicting");
  }

  this->graph_ptr_->SetInput(input_, input);
  output = this->graph_ptr_->Evaluate(output_);
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

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(14);

    // serialize the graph first
    map.Append(GRAPH, sp.graph_ptr_->GetGraphSaveableParams());

    map.Append(MODEL_CONFIG, sp.model_config_);

    map.Append(DATALOADER_TYPE, static_cast<uint8_t>(sp.dataloader_ptr_->LoaderCode()));

    // serialize dataloader
    switch (sp.dataloader_ptr_->LoaderCode())
    {
    case ml::LoaderType::TENSOR:
    {
      fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType> *loader_ptr =
          static_cast<fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType> *>(
              sp.dataloader_ptr_.get());
      map.Append(DATALOADER_PTR, *loader_ptr);
      break;
    }

    case ml::LoaderType::MNIST:
    case ml::LoaderType::SGNS:
    case ml::LoaderType::W2V:
    case ml::LoaderType::COMMODITY:
    case ml::LoaderType::C2V:
    {
      throw std::runtime_error("Serialization for current dataloader type not implemented yet.");
    }

    default:
    {
      throw std::runtime_error("Unknown dataloader type.");
    }
    }

    // serialize optimiser
    map.Append(OPTIMISER_TYPE, static_cast<uint8_t>(sp.optimiser_ptr_->OptimiserCode()));

    switch (sp.optimiser_ptr_->OptimiserCode())
    {
    case ml::OptimiserType::SGD:
    {
      fetch::ml::optimisers::SGDOptimiser<TensorType> *optimiser_ptr =
          static_cast<fetch::ml::optimisers::SGDOptimiser<TensorType> *>(sp.optimiser_ptr_.get());
      map.Append(OPTIMISER_PTR, *optimiser_ptr);
      break;
    }

    case ml::OptimiserType::ADAGRAD:
    case ml::OptimiserType::ADAM:
    case ml::OptimiserType::MOMENTUM:
    case ml::OptimiserType::RMSPROP:
    {
      throw std::runtime_error("serialization for current optimiser type not implemented yet.");
    }

    default:
    {
      throw std::runtime_error("Unknown optimiser type.");
    }
    }

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
    case ml::LoaderType::MNIST:
    case ml::LoaderType::SGNS:
    case ml::LoaderType::W2V:
    case ml::LoaderType::COMMODITY:
    case ml::LoaderType::C2V:
    {
      throw std::runtime_error("serialization for current dataloader type not implemented yet.");
    }

    default:
    {
      throw std::runtime_error("Unknown dataloader type.");
    }
    }

    // deserialize optimiser
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

    case ml::OptimiserType::ADAGRAD:
    case ml::OptimiserType::ADAM:
    case ml::OptimiserType::MOMENTUM:
    case ml::OptimiserType::RMSPROP:
    {
      throw std::runtime_error("serialization for current optimiser type not implemented yet.");
    }

    default:
    {
      throw std::runtime_error("Unknown optimiser type.");
    }
    }

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
