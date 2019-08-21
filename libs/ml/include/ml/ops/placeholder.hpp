#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "ml/ops/ops.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class PlaceHolder : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpPlaceholderSaveableParams<TensorType>;

  PlaceHolder() = default;

  explicit PlaceHolder(SPType const &sp)
    : Ops<T>(sp)
  {
    if (sp.output)
    {
      output_ = std::make_shared<TensorType>();
      output_->Resize(sp.output->shape());
      output_->Copy(*(sp.output));
    }
  }

  ~PlaceHolder() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType tp{};
    if (output_)
    {
      tp.output = std::make_shared<TensorType>(output_->Copy());
    }
    return std::make_shared<SPType>(tp);
  }

  /**
   * For placeholders should not be shared, therefore a layer sharing its elements
   * with another node should use a new (unshared) placeholder op
   * @param me
   * @return
   */
  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<PlaceHolder<TensorType>>();

    if (this->output_)
    {
      copyshare->output_ = std::make_shared<TensorType>(this->output_->shape());
    }

    return copyshare;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    assert(output_);
    output = *(output_);
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    return {error_signal};
  }

  virtual bool SetData(TensorType const &data)
  {
    bool shape_changed = true;
    if (output_)
    {
      shape_changed = (output_->shape() != data.shape());
    }
    output_ = std::make_shared<TensorType>(data);
    return shape_changed;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    FETCH_UNUSED(inputs);
    return output_->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_PLACEHOLDER;
  }

  static constexpr char const *DESCRIPTOR = "PlaceHolder";

protected:
  ArrayPtrType output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
