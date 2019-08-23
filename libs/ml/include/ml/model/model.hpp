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

#include "ml/model/model_config.hpp"
#include "ml/model/model_interface.hpp"
#include <vector>

namespace fetch {
namespace ml {
namespace model {

template <typename TensorType>
class Model : ModelInterface<TensorType>
{
public:
  using DataType          = typename TensorType::Type;
  using SizeType          = fetch::math::SizeType;
  using GraphType         = Graph<TensorType>;
  using DataLoaderType    = dataloaders::DataLoader<TensorType, TensorType>;
  using OptimiserType     = optimisers::Optimiser<TensorType>;
  using GraphPtrType      = typename std::shared_ptr<GraphType>;
  using DataLoaderPtrType = typename std::shared_ptr<DataLoaderType>;
  using OptimiserPtrType  = typename std::shared_ptr<OptimiserType>;

  /////////////////////////////
  /// Construct / Desctruct ///
  /////////////////////////////

  Model(DataLoaderPtrType data_loader_ptr, OptimiserType optimiser_type,
        ModelConfig<DataType> model_config, std::vector<SizeType> const &hidden_layers);

  explicit Model(ModelConfig<DataType> model_config = ModelConfig<DataType>())
    : ModelInterface<TensorType>(model_config)
  {}

  virtual ~Model() = default;

  /////////////////////////////
  ///
  /////////////////////////////
};

/**
 * constructor sets up the neural net architecture and optimiser
 * @tparam TensorType
 * @param data_loader_ptr  pointer to the dataloader for running the optimiser
 * @param model_config config parameters for setting up the network
 * @param hidden_layers vector of hidden layers for defining the architecture
 * @param optimiser_type type of optimiser to run
 */
template <typename TensorType>
Model<TensorType>::Model(DataLoaderPtrType data_loader_ptr, OptimiserType optimiser_type,
                         ModelConfig<DataType>        model_config,
                         std::vector<SizeType> const &hidden_layers)
  : ModelInterface<TensorType>(data_loader_ptr, optimiser_type, model_config)
{

  assert(!hidden_layers.empty());

  // instantiate feed forward network graph
  this->input_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  std::string cur_input = this->input_;
  for (SizeType cur_layer = 1; cur_layer < hidden_layers.size() - 1; ++cur_layer)
  {
    cur_input = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
        "", {cur_input}, hidden_layers.at(cur_layer - 1), hidden_layers.at(cur_layer),
        fetch::ml::details::ActivationType::RELU);
  }
  this->output_ = this->graph_ptr_->template AddNode<layers::FullyConnected<TensorType>>(
      "Output", {cur_input}, hidden_layers.at(hidden_layers.size() - 2),
      hidden_layers.at(hidden_layers.size() - 1), fetch::ml::details::ActivationType::SOFTMAX);

  this->label_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Label", {});
  this->error_ =
      this->graph_ptr_->template AddNode<CostFunctionType>("Error", {this->output_, this->label_});
}

}  // namespace model
}  // namespace ml
}  // namespace fetch
