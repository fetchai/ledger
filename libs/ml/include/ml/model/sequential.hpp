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
#include "ml/model/model_config.hpp"

#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace model {

template <typename TensorType>
class Sequential : public Model<TensorType>
{
public:
  using SizeType          = fetch::math::SizeType;
  using DataType          = typename TensorType::Type;
  using CostFunctionType  = fetch::ml::ops::CrossEntropyLoss<TensorType>;
  using OptimiserType     = fetch::ml::OptimiserType;
  using DataLoaderPtrType = typename Model<TensorType>::DataLoaderPtrType;

  Sequential();
  Sequential(Sequential const &other) = default;
  explicit Sequential(ModelConfig<DataType> model_config);
  ~Sequential() override = default;

  template <typename LayerType, typename... Params>
  void Add(Params... params);

  SizeType LayerCount() const;

  template <typename X, typename D>
  friend struct serializers::MapSerializer;

private:
  SizeType    layer_count_ = 0;
  std::string prev_layer_;
};

/**
 * constructor sets up the neural net architecture and optimiser with default config
 * @tparam TensorType
 * @param data_loader_ptr  pointer to the dataloader for running the optimiser
 * @param hidden_layers vector of hidden layers for defining the architecture
 * @param optimiser_type type of optimiser to run
 */
template <typename TensorType>
Sequential<TensorType>::Sequential()
  : Model<TensorType>(ModelConfig<DataType>())
{
  this->input_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  this->label_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Label", {});
}

/**
 * constructor sets up the neural net architecture and optimiser
 * @tparam TensorType
 * @param data_loader_ptr  pointer to the dataloader for running the optimiser
 * @param model_config config parameters for setting up the network
 * @param hidden_layers vector of hidden layers for defining the architecture
 * @param optimiser_type type of optimiser to run
 */
template <typename TensorType>
Sequential<TensorType>::Sequential(ModelConfig<DataType> model_config)
  : Model<TensorType>(model_config)
{
  this->input_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  this->label_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Label", {});
}

template <typename TensorType>
template <typename LayerType, typename... Params>
void Sequential<TensorType>::Add(Params... params)
{
  if (layer_count_ == 0)
  {
    // set as input layer
    prev_layer_   = this->graph_ptr_->template AddNode<LayerType>("", {this->input_}, params...);
    this->output_ = prev_layer_;
  }
  else
  {
    // set as output_layer (overwriting previous)
    prev_layer_   = this->output_;
    this->output_ = this->graph_ptr_->template AddNode<LayerType>("", {prev_layer_}, params...);
  }
  layer_count_++;
}

template <typename TensorType>
fetch::math::SizeType Sequential<TensorType>::LayerCount() const
{
  return layer_count_;
}

}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for Sequential model
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::model::Sequential<TensorType>, D>
{
  using Type                          = ml::model::Sequential<TensorType>;
  using DriverType                    = D;
  static uint8_t const BASE_MODEL     = 1;
  static uint8_t const LAYER_COUNT    = 2;
  static uint8_t const PREV_LAYER_STR = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(3);

    // serialize the optimiser parent class
    auto base_pointer = static_cast<ml::model::Model<TensorType> const *>(&sp);
    map.Append(BASE_MODEL, *base_pointer);
    map.Append(LAYER_COUNT, sp.layer_count_);
    map.Append(PREV_LAYER_STR, sp.prev_layer_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    auto base_pointer = static_cast<ml::model::Model<TensorType> *>(&sp);
    map.ExpectKeyGetValue(BASE_MODEL, *base_pointer);
    map.ExpectKeyGetValue(LAYER_COUNT, sp.layer_count_);
    map.ExpectKeyGetValue(PREV_LAYER_STR, sp.prev_layer_);
  }
};
}  // namespace serializers

}  // namespace fetch
