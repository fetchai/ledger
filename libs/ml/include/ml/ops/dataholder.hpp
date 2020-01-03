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
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
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

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    return std::make_shared<SPType>();
  }

  /**
   * forward recovers the stored data
   * @param inputs
   * @param output
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    assert(data_);
    output = *(data_);
  }

  /**
   * backward for non training dataholders should just pass back the error signal
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    return {error_signal};
  }

  /**
   * sets the internally stored data
   * @param data
   * @return
   */
  virtual bool SetData(TensorType const &data)
  {
    // TODO(VH): check for pre-set Shape and throw if it does not match.
    bool shape_changed = true;
    if (data_)
    {
      shape_changed = (data_->shape() != data.shape());
    }
    data_ = std::make_shared<TensorType>(data);
    return shape_changed;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    FETCH_UNUSED(inputs);
    assert(data_);
    return data_->shape();
  }

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
