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

#include "ml/ops/weights.hpp"

#include <cassert>
#include <memory>
#include <set>
#include <vector>

namespace fetch {
namespace ml {

struct OpsSaveableParams;

template <typename TensorType>
struct OpEmbeddingsSaveableParams;

namespace ops {

template <class T>
class Embeddings : public fetch::ml::ops::Weights<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Weights<T>::VecTensorType;
  using SPType        = OpEmbeddingsSaveableParams<TensorType>;
  using MyType        = Embeddings<TensorType>;

  Embeddings(SizeType dimensions, SizeType data_points);

  explicit Embeddings(TensorType const &weights);

  explicit Embeddings(SPType const &sp);

  ~Embeddings() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_EMBEDDINGS;
  }
  static constexpr char const *DESCRIPTOR = "Embedding";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
