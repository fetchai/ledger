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
class SkipGram : public SubGraph<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename T::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;

  SkipGram(SizeType in_size, SizeType out, SizeType embedding_size, SizeType vocab_size,
           std::string const &name = "SkipGram", WeightsInit init_mode = WeightsInit::XAVIER_GLOROT)
    : in_size_(in_size)
    , out_size_(out)
  {

    // define input and context placeholders
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});
    std::string context =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Context", {});

    ArrayType weights = std::vector<SizeType>({embedding_size, vocab_size});
    this->Initialise(weights, init_mode);

    // embed both inputs
    embed_in_ = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Inputs", {input}, weights);
    std::string embed_ctx = this->template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
        name + "_Embed_Context", {context}, weights);

    // dot product input and context embeddings
    std::string transpose_ctx = this->template AddNode<fetch::ml::ops::Transpose<ArrayType>>(
        name + "_TransposeCtx", {embed_ctx});

    std::string in_ctx_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
        name + "_In_Ctx_MatMul", {transpose_ctx, embed_in_});

    std::string in_ctx_matmul_flat = this->template AddNode<fetch::ml::ops::Flatten<ArrayType>>(
        name + "_In_Ctx_MatMul_Flat", {in_ctx_matmul});

    //    std::string in_ctx_matmul_flat = this->template
    //    AddNode<fetch::ml::layers::FullyConnected<ArrayType>>(
    //          name + "_In_Ctx_MatMul_Flat", {in_ctx_matmul}, in_size, out);

    std::string output = this->template AddNode<fetch::ml::ops::Sigmoid<ArrayType>>(
        name + "_Sigmoid", {in_ctx_matmul_flat});

    this->AddInputNode(input);
    this->AddInputNode(context);
    this->SetOutputNode(output);
  }

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    throw std::runtime_error("This shouldn't be called!");
  }

  std::shared_ptr<ops::Embeddings<ArrayType>> GetEmbeddings(std::shared_ptr<SkipGram<ArrayType>> &g)
  {
    return std::dynamic_pointer_cast<ops::Embeddings<ArrayType>>(g->GetNode(embed_in_));
  }

  std::string GetEmbedName()
  {
    return embed_in_;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.front()->shape().at(1), 1};
  }

  static constexpr char const *DESCRIPTOR = "SkipGram";

private:
  std::string embed_in_ = "";
  SizeType    in_size_;
  SizeType    out_size_;

  void Initialise(ArrayType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, in_size_, out_size_, init_mode);
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
