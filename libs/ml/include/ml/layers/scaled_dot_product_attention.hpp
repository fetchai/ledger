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

#include "math/standard_functions/sqrt.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/flatten.hpp"
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
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using DataType     = typename T::Type;
	using VecTensorType = typename SubGraph<T>::VecTensorType;
	
	ScaledDotProductAttention(std::uint64_t dk, DataType dropout = 0.1)
    : key_dim_(dk)
  {
		std::string name = DESCRIPTOR;
		
		// all input shapes are (feature_length, query/key/value_num, batch_num)
    std::string query =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Query", {});
    std::string key =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Key", {});
    std::string value =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Value", {});
		
    // Be advised that the matrix multiplication sequence is different from what is proposed in the paper
    // as our batch dimension is the last dimension, which the feature dimension is the first one.
    // in the paper, feature dimension is the col dimension
    // please refer to http://jalammar.github.io/illustrated-transformer/
    std::string transpose_key =
        this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(name + "_TransposeKey", {key});
    std::string kq_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_Key_Query_MatMul", {transpose_key, query});

    ArrayType sqrt_dk_tensor = std::vector<SizeType>({1, 1, 1});
    sqrt_dk_tensor(0, 0, 0)     = fetch::math::Sqrt(static_cast<DataType>(key_dim_));
    std::string sqrt_dk_ph   = this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(
        name + "_Sqrt_Key_Dim", {});
    this->SetInput(sqrt_dk_ph, sqrt_dk_tensor);
    
		// scale the QK matrix multiplication
    std::string scaled_kq_matmul = this->template AddNode<fetch::ml::ops::Divide<ArrayType>>(
        name + "_Scaled_Key_Query_MatMul", {kq_matmul, sqrt_dk_ph});

    // softmax
    std::string attention_weight = this->template AddNode<fetch::ml::ops::Softmax<ArrayType>>(
        name + "_Softmax", {scaled_kq_matmul}, 0);

    // dropout
    std::string dropout_attention_weight =
        this->template AddNode<fetch::ml::ops::Dropout<ArrayType>>(name + "_Dropout",
                                                                   {attention_weight}, dropout);

    // attention vectors
    std::string weight_value_matmul =
        this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
            name + "_Value_Weight_MatMul", {value, dropout_attention_weight});
    
    // in the end, the output is of shape (feature_length, query_num, batch_num)

    // TODO () masking op translation decoder

    this->AddInputNode(query);
    this->AddInputNode(key);
    this->AddInputNode(value);
    this->SetOutputNode(weight_value_matmul);
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return {inputs.front()->shape(0), inputs.at(2)->shape(1), inputs.front()->shape(2)};
  }

  static constexpr char const *DESCRIPTOR = "ScaledDotProductAttention";

private:
  SizeType key_dim_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
