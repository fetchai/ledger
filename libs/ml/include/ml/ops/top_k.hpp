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

#include "ml/ops/ops.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class TopK : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType     = T;
  using SizeType       = fetch::math::SizeType;
  using DataType       = typename TensorType::Type;
  using TensorSizeType = fetch::math::Tensor<SizeType>;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename Ops<T>::VecTensorType;
  using SPType         = OpTopKSaveableParams<T>;
  using MyType         = TopK<TensorType>;

  explicit TopK(SizeType k, bool sorted = true);

  explicit TopK(SPType const &sp);

  ~TopK() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_TOP_K;
  }
  static constexpr char const *DESCRIPTOR = "TopK";

private:
  SizeType k_;
  // For 2D input tensor we use data axis
  SizeType       axis_ = 0;
  bool           sorted_;
  TensorSizeType indices_;

  void UpdateIndices(VecTensorType const &inputs);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
