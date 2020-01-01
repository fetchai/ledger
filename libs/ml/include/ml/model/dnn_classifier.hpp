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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/model/model.hpp"
#include "ml/model/model_config.hpp"

#include <vector>

namespace fetch {
namespace ml {
namespace model {

template <typename TensorType>
class DNNClassifier : public Model<TensorType>
{
public:
  using SizeType          = fetch::math::SizeType;
  using DataType          = typename TensorType::Type;
  using CostFunctionType  = fetch::ml::ops::CrossEntropyLoss<TensorType>;
  using OptimiserType     = fetch::ml::OptimiserType;
  using DataLoaderPtrType = typename Model<TensorType>::DataLoaderPtrType;

  DNNClassifier()                           = default;
  DNNClassifier(DNNClassifier const &other) = default;
  ~DNNClassifier() override                 = default;

  DNNClassifier(ModelConfig<DataType> model_config, std::vector<SizeType> const &hidden_layers);
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
DNNClassifier<TensorType>::DNNClassifier(ModelConfig<DataType>        model_config,
                                         std::vector<SizeType> const &hidden_layers)
  : Model<TensorType>(model_config)
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
  this->loss_set_ = true;
}

}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for DNNClassifier model
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::model::DNNClassifier<TensorType>, D>
{
  using Type                      = ml::model::DNNClassifier<TensorType>;
  using DriverType                = D;
  static uint8_t const BASE_MODEL = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(1);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::model::Model<TensorType> const *>(&sp);
    map.Append(BASE_MODEL, *base_pointer);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::model::Model<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_MODEL, *base_pointer);
  }
};
}  // namespace serializers

}  // namespace fetch
