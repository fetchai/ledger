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
#include "ml/ops/add.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"
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
  
  std::string positionwise_feedforward(std::string name, std::string const &input){
  	// position wise feedforward with relu acitvation
	  std::string ff_first_layer = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	   name + "_Feedforward_No_1", {input}, static_cast<SizeType>(model_dim_),
	   static_cast<SizeType>(ff_dim_), ActivationType::RELU, RegType::NONE,
	   static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
	  
	  // do dropout
	  std::string ff_first_layer_dropout = this->template AddNode<fetch::ml::ops::Dropout>(name + "_Dropout", {ff_first_layer}, feedforward_dropout_);
	  
	  // position wise feedforward stage 2
	  std::string ff_second_layer = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
	   name + "_Feedforward_No_2", {ff_first_layer_dropout}, static_cast<SizeType>(ff_dim_),
	   static_cast<SizeType>(model_dim_), ActivationType::NOTHING, RegType::NONE,
	   static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
	  
	  return ff_second_layer;
  }
  
  std::string residual_connection(std::string name, std::string const &prev_layer_input, std::string const &prev_layer_output){
		
  	std::string dropout_output = this->template AddNode<fetch::ml::ops::Dropout>(name + "_Dropout", {prev_layer_output}, residual_dropout_);
	  std::string residual_addition = this->template AddNode<fetch::ml::ops::Add>(name + "_Residual_Addition", {prev_layer_input, dropout_output});
	  
  	std::string normalized_residual = this->template AddNode<fetch::ml::layers::LayerNorm<ArrayType>>(name + "_LayerNorm", {residual_addition});
  	
  }

  SelfAttentionEncoder(SizeType n_heads, SizeType model_dim, SizeType ff_dim, DataType residual_dropout = 0.9, DataType attention_dropout = 0.9, DataType feedforward_dropout = 0.9)
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

    // multihead attention on input time series vector
    std::string multihead_self_attention =
        this->template AddNode<fetch::ml::layers::MultiheadAttention<ArrayType>>(
            name + "_Multihead_Attention", {input, input, input}, n_heads, model_dim_, attention_dropout_);
    
    // make residual connection
    
    
    // do the final transformation
    std::string transformed_multihead =
        this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
            name + "_Final_Transformation", {concatenated_attention_heads},
            static_cast<SizeType>(model_dim_), static_cast<SizeType>(model_dim_),
            ActivationType::NOTHING, RegType::NONE, static_cast<DataType>(0),
            WeightsInitType::XAVIER_GLOROT, true);

    this->AddInputNode(query);
    this->AddInputNode(key);
    this->AddInputNode(value);
    this->SetOutputNode(transformed_multihead);
  }

  std::string create_one_attention_head(std::string const &head_name, std::string const &query,
                                        std::string const &key, std::string const &value)
  {
    // tansform input vectors to attention space
    std::string transformed_query =
        this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
            head_name + "_Query_Transform", {query}, static_cast<SizeType>(model_dim_),
            static_cast<SizeType>(key_dim_), ActivationType::NOTHING, RegType::NONE,
            static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
    std::string transformed_key =
        this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
            head_name + "_Query_Transform", {key}, static_cast<SizeType>(model_dim_),
            static_cast<SizeType>(key_dim_), ActivationType::NOTHING, RegType::NONE,
            static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
    std::string transformed_value =
        this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
            head_name + "_Query_Transform", {value}, static_cast<SizeType>(model_dim_),
            static_cast<SizeType>(value_dim_), ActivationType::NOTHING, RegType::NONE,
            static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, true);
    // do the scaled dot product attention
    std::string attention_output =
        this->template AddNode<fetch::ml::layers::ScaledDotProductAttention<ArrayType>>(
            head_name + "_Scaled_Dot_Product_Attention",
            {transformed_query, transformed_key, transformed_value}, key_dim_, dropout_);
    return attention_output;
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
