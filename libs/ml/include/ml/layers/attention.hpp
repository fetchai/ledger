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

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "math/standard_functions/sqrt.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Attention : public SubGraph<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using DataType     = typename T::Type;

  Attention(std::uint64_t dk, std::uint64_t dv, DataType dropout=0.9,
                std::string const &name = "Attention")
    : key_dim_(dk)
    , value_dim_(dv)
  {

    std::string query =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Query", {});
	  std::string key =
	   this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Key", {});
	  std::string value =
	   this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Value", {});
	
	  std::string transpose_key = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(
	   name + "_TransposeKey", {key});
	  std::string qk_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
	   name + "_Query_Key_MatMul", {query, transpose_key});
	
	  ArrayType sqrt_dv_tensor = std::vector<SizeType>({1, 1});
		sqrt_dv_tensor(0, 0) = fetch::math::Sqrt(static_cast<DataType>(key_dim_));
	  std::string sqrt_dv_ph =
	   this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Sqrt_Value_Dim", {}, sqrt_dv_tensor);
	  
	  std::string scaled_qk_matmul = this->template AddNode<fetch::ml::ops::Divide<ArrayType>>(
	   name + "_Scaled_Query_Key_MatMul", {qk_matmul, sqrt_dv_ph});
	
	  // softmax
	  std::string attention_weight =
	   this->template AddNode<fetch::ml::ops::Softmax<ArrayType>>(name + "_Softmax", {scaled_qk_matmul}, 0);
	  
	  // dropout
	  std::string dropout_attention_weight =
	   this->template AddNode<fetch::ml::ops::Dropout<ArrayType>>(name + "_Dropout", {attention_weight}, dropout);
		
	  // attention vectors
	  std::string weight_value_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
	   name + "_Weights_Value_MatMul", {dropout_attention_weight, value});
	
	  // TODO () masking op translation decoder

    this->AddInputNode(query);
	  this->AddInputNode(key);
	  this->AddInputNode(value);
    this->SetOutputNode(weight_value_matmul);
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Attention";

private:
  SizeType key_dim_;
  SizeType value_dim_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
