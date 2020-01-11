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
#include "ml/ops/embeddings.hpp"
#include "ml/ops/placeholder.hpp"

//#include <cmath>
//#include <random>

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
           WeightsInit        init_mode = WeightsInit::XAVIER_FAN_OUT);

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

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
