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
#include "ml/layers/layer.hpp"

#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"

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

    std::string input_name = this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});


    std::string query_name = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(name + "_Query", {input_name}, in, hidden);
    std::string key_name = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(name + "_Key", {input_name}, in, out);
    std::string value_name = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(name + "_Value", {input_name}, in, out);

    std::string flatten_query_name = this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_FlattenQuery", {query_name});
    std::string flatten_key_name = this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_FlattenKey", {key_name});
    std::string flatten_value_name = this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(name + "_FlattenValue", {value_name});

    // calculate attention on input vector
    std::string transpose_flatten_key_name = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(name + "_TransposeFlattenKey", {flatten_key_name});
    std::string qk_matmul_name = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(name + "_QKMatrixMultiply", {transpose_flatten_key_name, flatten_query_name});
    std::string softmax_name = this->template AddNode<fetch::ml::ops::Softmax<ArrayType>>(name + "_Softmax", {qk_matmul_name});

    // apply attention to input vector
    std::string output_name = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(name + "_AVMatrixMultiply", {softmax_name, value_name});

    this->AddInputNodes(input_name);
    this->SetOutputNode(output_name);
  }

  static constexpr char const *DESCRIPTOR = "SelfAttention";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
