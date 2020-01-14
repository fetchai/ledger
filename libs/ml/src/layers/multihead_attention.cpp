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

#include "ml/layers/multihead_attention.hpp"

#include "ml/ops/add.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <string>

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
MultiheadAttention<TensorType>::MultiheadAttention(SizeType n_heads, SizeType model_dim,
                                                   DataType dropout)
  : n_heads_(n_heads)
  , model_dim_(model_dim)
  , dropout_(dropout)
{
  // make sure all heads can be concatenated together to form model_dim
  assert(model_dim_ % n_heads_ == 0);
  key_dim_ = model_dim_ / n_heads_;

  // assuming key_dim is the same as value_dim
  value_dim_ = key_dim_;

  std::string name = DESCRIPTOR;

  // all input shapes are (feature_length, model_dim, batch_num)
  std::string query =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Query", {});
  std::string key =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Key", {});
  std::string value =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Value", {});
  std::string mask =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Mask", {});

  // do n_heads time linear transformation
  std::vector<std::string> heads;
  for (SizeType i = 0; i < n_heads_; i++)
  {
    std::string head_name = name + "_Head_No_" + std::to_string(static_cast<int>(i));
    std::string head_attention_output =
        create_one_attention_head(head_name, query, key, value, mask);
    heads.emplace_back(head_attention_output);
  }

  // concatenate all attention head outputs
  std::string concatenated_attention_heads =
      this->template AddNode<fetch::ml::ops::Concatenate<TensorType>>(
          name + "_Concatenated_Heads", heads, static_cast<SizeType>(0));
  // do the final transformation
  std::string transformed_multihead =
      this->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          name + "_Final_Transformation", {concatenated_attention_heads},
          static_cast<SizeType>(model_dim_), static_cast<SizeType>(model_dim_),
          ActivationType::NOTHING, RegType::NONE, DataType{0}, WeightsInitType::XAVIER_GLOROT,
          true);

  this->AddInputNode(query);
  this->AddInputNode(key);
  this->AddInputNode(value);
  this->AddInputNode(mask);
  this->SetOutputNode(transformed_multihead);
  this->Compile();
}

template <typename TensorType>
std::string MultiheadAttention<TensorType>::create_one_attention_head(std::string const &head_name,
                                                                      std::string const &query,
                                                                      std::string const &key,
                                                                      std::string const &value,
                                                                      std::string const &mask)
{
  // transform input vectors to attention space
  std::string transformed_query =
      this->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          head_name + "_Query_Transform", {query}, static_cast<SizeType>(model_dim_),
          static_cast<SizeType>(key_dim_), ActivationType::NOTHING, RegType::NONE, DataType{0},
          WeightsInitType::XAVIER_GLOROT, true);
  std::string transformed_key =
      this->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          head_name + "_Key_Transform", {key}, static_cast<SizeType>(model_dim_),
          static_cast<SizeType>(key_dim_), ActivationType::NOTHING, RegType::NONE, DataType{0},
          WeightsInitType::XAVIER_GLOROT, true);
  std::string transformed_value =
      this->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
          head_name + "_Value_Transform", {value}, static_cast<SizeType>(model_dim_),
          static_cast<SizeType>(value_dim_), ActivationType::NOTHING, RegType::NONE, DataType{0},
          WeightsInitType::XAVIER_GLOROT, true);
  // do the scaled dot product attention
  std::string attention_output =
      this->template AddNode<fetch::ml::layers::ScaledDotProductAttention<TensorType>>(
          head_name + "_Scaled_Dot_Product_Attention",
          {transformed_query, transformed_key, transformed_value, mask}, key_dim_, dropout_);
  return attention_output;
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> MultiheadAttention<TensorType>::GetOpSaveableParams()
{
  auto ret = std::make_shared<SPType>();
  // get base class saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  // assign base class saveable params to ret
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::static_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  // asign layer specific params
  ret->key_dim   = key_dim_;
  ret->value_dim = value_dim_;
  ret->n_heads   = n_heads_;
  ret->model_dim = model_dim_;
  ret->dropout   = dropout_;
  return ret;
}

template <typename TensorType>
void MultiheadAttention<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  key_dim_   = sp.key_dim;
  value_dim_ = sp.value_dim;
  n_heads_   = sp.n_heads;
  model_dim_ = sp.model_dim;
  dropout_   = sp.dropout;
}

template <typename TensorType>
std::vector<math::SizeType> MultiheadAttention<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  return inputs.front()->shape();
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// template class MultiheadAttention<math::Tensor<int8_t>>;
// template class MultiheadAttention<math::Tensor<int16_t>>;
template class MultiheadAttention<math::Tensor<int32_t>>;
template class MultiheadAttention<math::Tensor<int64_t>>;
template class MultiheadAttention<math::Tensor<float>>;
template class MultiheadAttention<math::Tensor<double>>;
template class MultiheadAttention<math::Tensor<fixed_point::fp32_t>>;
template class MultiheadAttention<math::Tensor<fixed_point::fp64_t>>;
template class MultiheadAttention<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
