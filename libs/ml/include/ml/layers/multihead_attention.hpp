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
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/placeholder.hpp"

#include <string>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class MultiheadAttention : public SubGraph<T>
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

  MultiheadAttention(SizeType n_heads, SizeType model_dim, DataType dropout = 0.9)
    : n_heads_(n_heads)
    , model_dim_(model_dim)
    , dropout_(dropout)
  {
    // make sure all heads can be concatenate together to form model_dim
    assert(model_dim_ % n_heads_ == 0);
    key_dim_ = model_dim_ / n_heads_;

    // assuming key_dim is the same as value_dim
    value_dim_ = key_dim_;

    std::string name = DESCRIPTOR;

    // all input shapes are (feature_length, model_dim, batch_num)
    std::string query =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Query", {});
    std::string key =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Key", {});
    std::string value =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Value", {});

    // do n_heads time linear transformation
    std::vector<std::string> heads;
    for (SizeType i{0}; i < n_heads_; i++)
    {
      std::string head_name             = name + "_Head_No_" + std::to_string(static_cast<int>(i));
      std::string head_attention_output = create_one_attention_head(head_name, query, key, value);
      heads.emplace_back(head_attention_output);
    }

    // concatenate all attention head outputs
    std::string concatenated_attention_heads =
        this->template AddNode<fetch::ml::ops::Concatenate<ArrayType>>(
            name + "_Concatenated_Heads", heads, static_cast<SizeType>(0));
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

  static constexpr char const *DESCRIPTOR = "MultiheadAttention";

private:
  SizeType key_dim_;
  SizeType value_dim_;
  SizeType n_heads_;
  SizeType model_dim_;
  DataType dropout_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
