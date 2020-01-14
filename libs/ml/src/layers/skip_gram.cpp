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

#include "ml/layers/skip_gram.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/transpose.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
SkipGram<TensorType>::SkipGram(SizeType in_size, SizeType out, SizeType embedding_size,
                               SizeType vocab_size, std::string const &name, WeightsInit init_mode)
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
  embed_in_ = this->template AddNode<fetch::ml::ops::Embeddings<TensorType>>(name + "_Embed_Inputs",
                                                                             {input}, weights_in);
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

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> SkipGram<TensorType>::GetOpSaveableParams()
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

template <typename TensorType>
void SkipGram<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  out_size_   = sp.out_size;
  embed_in_   = sp.embed_in;
  vocab_size_ = sp.vocab_size;
}

// template class SkipGram<math::Tensor<int8_t>>;
// template class SkipGram<math::Tensor<int16_t>>;
template class SkipGram<math::Tensor<int32_t>>;
template class SkipGram<math::Tensor<int64_t>>;
template class SkipGram<math::Tensor<float>>;
template class SkipGram<math::Tensor<double>>;
template class SkipGram<math::Tensor<fixed_point::fp32_t>>;
template class SkipGram<math::Tensor<fixed_point::fp64_t>>;
template class SkipGram<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
