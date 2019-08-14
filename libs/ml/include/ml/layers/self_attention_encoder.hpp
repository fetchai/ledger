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

#include "layers.hpp"
#include "math/standard_functions/sqrt.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/placeholder.hpp"
#include "multihead_attention.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <ml/ops/concatenate.hpp>
#include <random>
#include <string>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class SelfAttentionEncoder : public SubGraph<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using DataType      = typename T::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;

  using RegType         = fetch::ml::details::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;

  SelfAttentionEncoder(SizeType n_heads, SizeType model_dim, SizeType ff_dim,
                       DataType residual_dropout    = static_cast<DataType>(0.9),
                       DataType attention_dropout   = static_cast<DataType>(0.9),
                       DataType feedforward_dropout = static_cast<DataType>(0.9))
    : n_heads_(n_heads)
    , model_dim_(model_dim)
    , ff_dim_(ff_dim)
    , residual_dropout_(residual_dropout)
    , attention_dropout_(attention_dropout)
    , feedforward_dropout_(feedforward_dropout)
  {
    // make sure all heads can be concatenate together to form model_dim
    assert(model_dim_ % n_heads_ == 0);

    std::string name = DESCRIPTOR;

    // all input shapes are (feature_length, model_dim, batch_num)
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    std::string mask =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "Mask", {});

    // multihead attention on input time series vector
    std::string multihead_self_attention =
        this->template AddNode<fetch::ml::layers::MultiheadAttention<ArrayType>>(
            name + "_Multihead_Attention", {input, input, input, mask}, n_heads, model_dim_,
            attention_dropout_);

    // make residual connection
    std::string attention_residual =
        residual_connection(name + "_Attention_Residual", input, multihead_self_attention);

    // do feed forward
    std::string feedforward = positionwise_feedforward(name + "_Feedforward", attention_residual);

    // make residual connection
    std::string feedforward_residual =
        residual_connection(name + "_Feedforward_Residual", attention_residual, feedforward);

    this->AddInputNode(input);
    this->AddInputNode(mask);
    this->SetOutputNode(feedforward_residual);
  }

  std::string positionwise_feedforward(std::string name, std::string const &input)
  {
    // position wise feedforward with gelu acitvation
    std::string ff_first_layer =
        this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
            name + "_Feedforward_No_1", {input}, static_cast<SizeType>(model_dim_),
            static_cast<SizeType>(ff_dim_), ActivationType::RELU, RegType::NONE,
            static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);

    // do dropout
    std::string ff_first_layer_dropout = this->template AddNode<fetch::ml::ops::Dropout<ArrayType>>(
        name + "_Dropout", {ff_first_layer}, feedforward_dropout_);

    // position wise feedforward stage 2
    std::string ff_second_layer =
        this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
            name + "_Feedforward_No_2", {ff_first_layer_dropout}, static_cast<SizeType>(ff_dim_),
            static_cast<SizeType>(model_dim_), ActivationType::NOTHING, RegType::NONE,
            static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);

    return ff_second_layer;
  }

  std::string residual_connection(std::string name, std::string const &prev_layer_input,
                                  std::string const &prev_layer_output)
  {
    // do a dropout of prev output before doing residual connection
    std::string dropout_output = this->template AddNode<fetch::ml::ops::Dropout<ArrayType>>(
        name + "_Dropout", {prev_layer_output}, residual_dropout_);
    std::string residual_addition = this->template AddNode<fetch::ml::ops::Add<ArrayType>>(
        name + "_Residual_Addition", {prev_layer_input, dropout_output});

    // create sudo shape for layernorm
    std::vector<SizeType> data_shape({model_dim_, 1});
    std::string           normalized_residual =
        this->template AddNode<fetch::ml::layers::LayerNorm<ArrayType>>(
            name + "_LayerNorm", {residual_addition}, data_shape, static_cast<SizeType>(0));

    return normalized_residual;
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "SelfAttentionEncoder";

private:
  SizeType n_heads_;
  SizeType model_dim_;
  SizeType ff_dim_;
  DataType residual_dropout_;
  DataType attention_dropout_;
  DataType feedforward_dropout_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
