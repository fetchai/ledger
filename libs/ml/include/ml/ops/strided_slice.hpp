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
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class StridedSlice : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = fetch::math::SizeVector;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpStridedSliceSaveableParams<T>;
  using MyType        = StridedSlice<TensorType>;

  explicit StridedSlice(SizeVector const &begins, SizeVector const &ends,
                        SizeVector const &strides = {});

  explicit StridedSlice(SPType const &sp);

  ~StridedSlice() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  SizeVector ComputeOutputShape(std::vector<math::SizeVector> const &inputs) const override;

  SizeVector begins_;
  SizeVector ends_;
  SizeVector strides_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_STRIDED_SLICE;
  }

  OperationsCount                              ChargeForward() const override;
  std::pair<OperationsCount, math::SizeVector> ChargeBackward(
      std::vector<math::SizeVector> const &input_shapes) override;

  static constexpr char const *DESCRIPTOR = "StridedSlice";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
