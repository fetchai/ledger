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
#include "ml/ops/activations/sigmoid.hpp"
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
    std::string embed_in = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Input", {input}, in_size, embedding_size);
    std::string embed_ctx = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Context", {context}, in_size, embedding_size);

    // dot product input and context embeddings
    std::string transpose_ctx = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(
        name + "_TransposeCtx", {embed_ctx});
    std::string in_ctx_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_In_Ctx_MatMul", {embed_in, transpose_ctx});

    // sigmoid activation
    std::string output = this->template AddNode<fetch::ml::ops::Sigmoid<ArrayType>>(
        name + "_Output", {in_ctx_matmul});

    this->AddInputNodes(input);
    this->SetOutputNode(output);

    // set up data for embeddings
    ArrayPtrType embeddings_ptr =
        std::make_shared<ArrayType>(std::vector<std::uint64_t>({in_size, embedding_size}));
    this->Initialise(embeddings_ptr, init_mode);
    this->SetInput(embed_in, embeddings_ptr);
    this->SetInput(embed_ctx, embeddings_ptr);
  }

  static constexpr char const *DESCRIPTOR = "SkipGram";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
