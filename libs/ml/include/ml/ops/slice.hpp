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
class Slice : public Ops<T>
{
public:
  using TensorType     = T;
  using SizeType       = fetch::math::SizeType;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename Ops<T>::VecTensorType;
  using ConstSliceType = typename TensorType::ConstSliceType;
  using SPType         = OpSliceSaveableParams<T>;
  using MyType         = Slice<TensorType>;

  explicit Slice(std::vector<SizeType> indices, std::vector<SizeType> axes);

  explicit Slice(SizeType index, SizeType axis);

  explicit Slice(std::pair<SizeType, SizeType> start_end_slice, SizeType axis);

  explicit Slice(SPType const &sp);

  ~Slice() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  std::pair<SizeType, SizeType> start_end_slice_;
  std::vector<SizeType>         axes_;
  std::vector<SizeType>         indices_;
  SizeType                      axis_;
  SizeType                      index_;
  TensorType                    ret_error_signal_;

  SliceType slice_type_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_SLICE;
  }

  static constexpr char const *DESCRIPTOR = "Slice";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
