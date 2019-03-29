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
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/transpose.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {

template <typename T>
class Ops;

namespace layers {

template <class T>
class SkipGram : public Layer<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename T::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  SkipGram(SizeType in_size, SizeType out, SizeType embedding_size, SizeType vocab_size,
           std::string const &name = "SkipGram", WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
    : Layer<T>(in_size, out)
  {

    // define input and context placeholders
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    std::string context =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Context", {});

    ArrayType weights = std::vector<SizeType>({vocab_size, embedding_size});
    this->Initialise(weights, init_mode);

    // embed both inputs
    embed_in_ = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Inputs", {input}, in_size, embedding_size, weights);
    std::string embed_ctx = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Context", {context}, in_size, embedding_size, weights);

    // dot product input and context embeddings
    std::string transpose_ctx = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(
        name + "_TransposeCtx", {embed_ctx});
    std::string in_ctx_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_In_Ctx_MatMul", {embed_in_, transpose_ctx});

    // dense layer
    std::string fc_out = this->template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
        name + "_Dense", {in_ctx_matmul}, in_size, out);

    // sigmoid activation
    std::string output =
        this->template AddNode<fetch::ml::ops::Sigmoid<ArrayType>>(name + "_Sigmoid", {fc_out});

    this->AddInputNode(input);
    this->AddInputNode(context);
    this->SetOutputNode(output);
  }

  // Overload that method for optimisation purposes
  virtual ArrayType ForwardBatch(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    std::vector<ArrayType> results;
    for (typename ArrayType::SizeType b(0); b < inputs.front().get().shape()[0]; ++b)
    {
      ArrayType slice_input   = inputs.front().get().Slice(b);
      ArrayType slice_context = inputs.back().get().Slice(b);
      results.push_back(this->Forward({slice_input, slice_context}));
    }
    return ConcatenateTensors(results);
    //    return this->fetch::ml::Ops<T>::ForwardBatch(inputs);
  }

  std::shared_ptr<ops::Embeddings<ArrayType>> GetEmbeddings(std::shared_ptr<SkipGram<ArrayType>> &g)
  {
    return std::dynamic_pointer_cast<ops::Embeddings<ArrayType>>(g->GetNode(embed_in_));
  }

  std::string GetEmbedName()
  {
    return embed_in_;
  }

  static constexpr char const *DESCRIPTOR = "SkipGram";

private:
  std::string embed_in_ = "";
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
