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
class SelfAttention : public SubGraph<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = SelfAttentionSaveableParams<TensorType>;

  SelfAttention() = default;

  SelfAttention(std::uint64_t in, std::uint64_t out, std::uint64_t hidden,
                std::string const &name = "SA")
    : in_size_(in)
    , out_size_(out)
  {

    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    // key & value dense layer
    std::string flat_input = this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(
        name + "_Flatten_Input", {input});
    std::string query_name = this->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        name + "_KEY_VAL", {flat_input}, in, hidden);

    //////////////////////
    /// attention part ///
    //////////////////////

    // query key matmul
    std::string transpose_key = this->template AddNode<fetch::ml::ops::Transpose<TensorType>>(
        name + "_TransposeKey", {flat_input});
    std::string qk_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
        name + "_Query_Key_MatMul", {flat_input, transpose_key});

    // TODO(707) normalise by square root of vector length

    // softmax
    std::string attention_weights =
        this->template AddNode<fetch::ml::ops::Softmax<TensorType>>(name + "_Softmax", {qk_matmul});

    // attention & value matmul
    std::string weighted_value = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
        name + "_Att_Val_MatMul", {attention_weights, flat_input});

    // residual connection to input
    std::string decoding = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
        name + "_ResidualConnection", {flat_input, weighted_value});

    // final dense output
    std::string output = this->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
        name + "_OutputFC", {decoding}, in, out);

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    // get base class saveable params
    std::shared_ptr<SaveableParamsInterface> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    auto ret = std::make_shared<SPType>();

    // copy graph saveable params over
    auto g_ptr1 = std::dynamic_pointer_cast<typename Graph<TensorType>::SPType>(sgsp);
    auto g_ptr2 = std::dynamic_pointer_cast<typename Graph<TensorType>::SPType>(ret);
    *g_ptr2     = *g_ptr1;

    // copy subgraph saveable params over
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // asign layer specific params
    ret->in_size  = in_size_;
    ret->out_size = out_size_;
    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    in_size_  = sp.in_size;
    out_size_ = sp.out_size;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_SELF_ATTENTION;
  }

  static constexpr char const *DESCRIPTOR = "SelfAttention";

private:
  SizeType in_size_  = fetch::math::numeric_max<SizeType>();
  SizeType out_size_ = fetch::math::numeric_max<SizeType>();
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
