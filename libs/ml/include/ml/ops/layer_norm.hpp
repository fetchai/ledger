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
#include <memory>
#include <vector>

namespace fetch {
namespace ml {

struct OpsSaveableParams;

template <typename TensorType>
struct OpLayerNormSaveableParams;

namespace ops {

template <class T>
class LayerNorm : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpLayerNormSaveableParams<T>;
  using MyType        = LayerNorm<TensorType>;

  explicit LayerNorm(SizeType axis    = static_cast<SizeType>(0),
                     DataType epsilon = fetch::math::function_tolerance<DataType>());

  explicit LayerNorm(SPType const &sp);

  ~LayerNorm() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_LAYER_NORM;
  }

  static constexpr char const *DESCRIPTOR = "LayerNormalization";

private:
  SizeType axis_;
  DataType epsilon_;

  TensorType prev_input_;
  TensorType cached_inv_sqrt_var_;
  TensorType cached_output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
