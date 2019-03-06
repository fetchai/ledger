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

#include "ml/layers/layer.hpp"

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class SelfAttention : public Layer<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  SelfAttention(std::uint64_t in, std::uint64_t out, std::uint64_t hidden,
                std::string const &name = "SA")
    : Layer<T>(in, out)
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

    this->AddInputNodes(input);
    this->SetOutputNode(output);
  }

  static constexpr char const *DESCRIPTOR = "SelfAttention";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
