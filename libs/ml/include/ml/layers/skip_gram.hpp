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

#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/layer.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class SkipGram : public Layer<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename T::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  SkipGram(SizeType in_size, SizeType out, SizeType embedding_size,
           std::string const &name = "SkipGram", WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
    : Layer<T>(in_size, out)
  {

    // define input and context placeholders
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    std::string context =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Context", {});

    // embed both inputs
    embed_in_ = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Input", {input}, in_size, embedding_size);
    std::string embed_ctx = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Context", {context}, in_size, embedding_size);

    // dot product input and context embeddings
    std::string transpose_ctx = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(
        name + "_TransposeCtx", {embed_ctx});
    std::string in_ctx_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_In_Ctx_MatMul", {transpose_ctx, embed_in_});

    // dense layer
    SizeType    dense_size = fetch::math::Square(embedding_size);
    std::string dense      = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
        name + "_Dense", {in_ctx_matmul}, dense_size, out);

    // softmax activation
    std::string output =
        this->template AddNode<fetch::ml::ops::Softmax<ArrayType>>(name + "_Output", {dense});

    this->AddInputNode(input);
    this->AddInputNode(context);
    this->SetOutputNode(output);

    ArrayPtrType weights_ptr =
        std::make_shared<ArrayType>(std::vector<SizeType>({in_size, embedding_size}));
    this->Initialise(weights_ptr, init_mode);
    this->SetInput(embed_in_, weights_ptr);
    this->SetInput(embed_ctx, weights_ptr);
  }

  std::shared_ptr<ops::Embeddings<ArrayType>> GetEmbeddings(std::shared_ptr<SkipGram<ArrayType>> &g)
  {
    return std::dynamic_pointer_cast<ops::Embeddings<ArrayType>>(g->GetNode(embed_in_));
  }

  static constexpr char const *DESCRIPTOR = "SkipGram";

private:
  std::string embed_in_ = "";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
