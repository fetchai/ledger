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

#include "math/standard_functions/sqrt.hpp"
#include "ml/core/subgraph.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/constant.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/mask_fill.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class ScaledDotProductAttention : public SubGraph<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using DataType      = typename T::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerScaledDotProductAttentionSaveableParams<T>;

  ScaledDotProductAttention() = default;
  explicit ScaledDotProductAttention(SizeType dk,
                                     DataType dropout = fetch::math::Type<DataType>("0.9"))
    : key_dim_(dk)
    , dropout_(dropout)
  {
    std::string name = DESCRIPTOR;

    // all input shapes are (feature_length, query/key/value_num, batch_num)
    std::string query =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Query", {});
    std::string key =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Key", {});
    std::string value =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Value", {});
    std::string mask =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Mask", {});

    // Be advised that the matrix multiplication sequence is different from what is proposed in the
    // paper as our batch dimension is the last dimension, which the feature dimension is the first
    // one. in the paper, feature dimension is the col dimension please refer to
    // http://jalammar.github.io/illustrated-transformer/
    std::string transpose_key = this->template AddNode<fetch::ml::ops::Transpose<TensorType>>(
        name + "_TransposeKey", {key});
    std::string kq_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
        name + "_Key_Query_MatMul", {transpose_key, query});

    TensorType sqrt_dk_tensor(std::vector<SizeType>({1, 1, 1}));
    sqrt_dk_tensor(0, 0, 0) = fetch::math::Sqrt(static_cast<DataType>(key_dim_));
    std::string sqrt_dk_ph =
        this->template AddNode<fetch::ml::ops::Constant<TensorType>>(name + "_Sqrt_Key_Dim", {});
    this->SetInput(sqrt_dk_ph, sqrt_dk_tensor);

    // scale the QK matrix multiplication
    std::string scaled_kq_matmul = this->template AddNode<fetch::ml::ops::Divide<TensorType>>(
        name + "_Scaled_Key_Query_MatMul", {kq_matmul, sqrt_dk_ph});

    // masking: make sure you mask along the feature dimension if the mask is to be broadcasted
    std::string masked_scaled_kq_matmul =
        this->template AddNode<fetch::ml::ops::MaskFill<TensorType>>(
            name + "_Masking", {mask, scaled_kq_matmul}, DataType{-1000000000});

    // softmax
    std::string attention_weight = this->template AddNode<fetch::ml::ops::Softmax<TensorType>>(
        name + "_Softmax", {masked_scaled_kq_matmul}, static_cast<SizeType>(0));

    // dropout
    std::string dropout_attention_weight =
        this->template AddNode<fetch::ml::ops::Dropout<TensorType>>(name + "_Dropout",
                                                                    {attention_weight}, dropout_);
    // attention vectors
    std::string weight_value_matmul =
        this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
            name + "_Value_Weight_MatMul", {value, dropout_attention_weight});

    // in the end, the output is of shape (feature_length, query_num, batch_num)

    this->AddInputNode(query);
    this->AddInputNode(key);
    this->AddInputNode(value);
    this->AddInputNode(mask);
    this->SetOutputNode(weight_value_matmul);
    this->Compile();
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret = std::make_shared<SPType>();
    // get base class saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    // assign base class saveable params to ret
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::static_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // asign layer specific params
    ret->key_dim = key_dim_;
    ret->dropout = dropout_;
    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    // assign layer specific params
    key_dim_ = sp.key_dim;
    dropout_ = sp.dropout;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.at(2)->shape(0), inputs.front()->shape(1), inputs.front()->shape(2)};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION;
  }

  static constexpr char const *DESCRIPTOR = "ScaledDotProductAttention";

private:
  SizeType key_dim_{};
  DataType dropout_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
