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
struct OpDataHolderSaveableParams;

namespace ops {

/**
 * DataHolder is an abstract class for sharing implementations between
 * Constant, Variable, and PlaceHolder
 * @tparam T
 */

template <class T>
class DataHolder : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using TensorPtrType = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpDataHolderSaveableParams<T>;
  using MyType        = DataHolder<TensorType>;

  DataHolder() = default;

  explicit DataHolder(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~DataHolder() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  virtual bool SetData(TensorType const &data);

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_DATAHOLDER;
  }

  static constexpr char const *DESCRIPTOR = "DataHolder";

protected:
  TensorPtrType data_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
