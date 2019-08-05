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
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = SelfAttentionSaveableParams<ArrayType>;

  SelfAttention(std::uint64_t in, std::uint64_t out, std::uint64_t hidden,
                std::string const &name = "SA")
    : in_size_(in)
    , out_size_(out)
  {

    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

    // key & value dense layer
    std::string flat_input = this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(
        name + "_Flatten_Input", {input});
    std::string query_name = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
        name + "_KEY_VAL", {flat_input}, in, hidden);

    //////////////////////
    /// attention part ///
    //////////////////////

    // query key matmul
    std::string transpose_key = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(
        name + "_TransposeKey", {flat_input});
    std::string qk_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_Query_Key_MatMul", {flat_input, transpose_key});

    // TODO(707) normalise by square root of vector length

    // softmax
    std::string attention_weights =
        this->template AddNode<fetch::ml::ops::Softmax<ArrayType>>(name + "_Softmax", {qk_matmul});

    // attention & value matmul
    std::string weighted_value = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_Att_Val_MatMul", {attention_weights, flat_input});

    // residual connection to input
    std::string decoding = this->template AddNode<fetch::ml::ops::Add<ArrayType>>(
        name + "_ResidualConnection", {flat_input, weighted_value});

    // final dense output
    std::string output = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
        name + "_OutputFC", {decoding}, in, out);

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    auto sp               = std::make_shared<SPType>();
    auto ret_sgsp_pointer = std::dynamic_pointer_cast<std::shared_ptr<SubGraphSaveableParams<ArrayType>>>(sp);

    auto base_pointer  = std::dynamic_pointer_cast<SubGraph<ArrayType>>(this);
    auto this_sgsp_ptr = base_pointer->GetOpSaveableParams();

    *ret_sgsp_pointer = *this_sgsp_ptr;

    sp->in_size  = in_size_;
    sp->out_size = out_size_;
    return sp;
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
  SizeType in_size_;
  SizeType out_size_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
