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

#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class OneHot : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpOneHotSaveableParams<T>;
  using MyType        = OneHot<TensorType>;

  /**
   * One hot function based on tf.one_hot
   * @param depth number of classes
   * @param axis
   * @param on_value TRUE value
   * @param off_value FALSE value
   */
  explicit OneHot(SizeType depth, SizeType axis = 0, DataType on_value = DataType{1},
                  DataType off_value = DataType{0})
    : depth_(depth)
    , axis_(axis)
    , on_value_(on_value)
    , off_value_(off_value)
  {}

  explicit OneHot(SPType const &sp);

  ~OneHot() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;
  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_ONE_HOT;
  }
  static constexpr char const *DESCRIPTOR = "OneHot";

private:
  SizeType depth_;
  SizeType axis_;
  DataType on_value_  = DataType{1};
  DataType off_value_ = DataType{0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
