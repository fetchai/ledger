#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/core/subgraph.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/flatten.hpp"
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
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerSkipGramSaveableParams<T>;

  SkipGram() = default;

  SkipGram(SizeType in_size, SizeType out, SizeType embedding_size, SizeType vocab_size,
           std::string const &name      = "SkipGram",
           WeightsInit        init_mode = WeightsInit::XAVIER_FAN_OUT)
    : out_size_(out)
    , vocab_size_(vocab_size)
  {

    // define input and context placeholders
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});
    std::string context =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Context", {});

    TensorType weights_in({embedding_size, vocab_size_});
    this->Initialise(weights_in, init_mode, in_size, embedding_size);
    TensorType weights_ctx({embedding_size, vocab_size_});
    this->Initialise(weights_ctx, init_mode, in_size, embedding_size);

    // embed both inputs
    embed_in_ = this->template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
        name + "_Embed_Inputs", {input}, weights_in);
    std::string embed_ctx = this->template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
        name + "_Embed_Context", {context}, weights_ctx);

    // dot product input and context embeddings
    std::string in_ctx_matmul = this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
        name + "_In_Ctx_MatMul", {embed_ctx, embed_in_}, true);

    std::string in_ctx_matmul_flat = this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(
        name + "_In_Ctx_MatMul_Flat", {in_ctx_matmul});

    std::string output = this->template AddNode<fetch::ml::ops::Sigmoid<TensorType>>(
        name + "_Sigmoid", {in_ctx_matmul_flat});

    this->AddInputNode(input);
    this->AddInputNode(context);
    this->SetOutputNode(output);
    this->Compile();
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    // get all base classes saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    auto ret = std::make_shared<SPType>();

    // copy subgraph saveable params over
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // assign layer specific params
    ret->out_size   = out_size_;
    ret->embed_in   = embed_in_;
    ret->vocab_size = vocab_size_;

    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    // assign layer specific params
    out_size_   = sp.out_size;
    embed_in_   = sp.embed_in;
    vocab_size_ = sp.vocab_size;
  }

  std::shared_ptr<ops::Embeddings<TensorType>> GetEmbeddings(
      std::shared_ptr<SkipGram<TensorType>> &g)
  {
    return std::dynamic_pointer_cast<ops::Embeddings<TensorType>>((g->GetNode(embed_in_))->GetOp());
  }

  std::string GetEmbedName()
  {
    return embed_in_;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.front()->shape().at(1), 1};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_SKIP_GRAM;
  }

  static constexpr char const *DESCRIPTOR = "SkipGram";

private:
  std::string embed_in_ = "";
  SizeType    out_size_{};
  SizeType    vocab_size_{};

  void Initialise(TensorType &weights, WeightsInit init_mode, SizeType dim_1_size,
                  SizeType dim_2_size)
  {
    fetch::ml::ops::Weights<TensorType>::Initialise(weights, dim_1_size, dim_2_size, init_mode);
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
