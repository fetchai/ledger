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

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class PlaceHolder : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  PlaceHolder() = default;

  explicit PlaceHolder(TrainableSaveableParams<ArrayType> tp)
  {
    output_ = tp.weights;
  }

  std::shared_ptr<SaveableParams<ArrayType>> GetOpSaveableParams()
  {
    TrainableSaveableParams<ArrayType> tp{};
    tp.weights_   = output_;
    tp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<TrainableSaveableParams<ArrayType>>(tp);
  }

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    assert(this->output_);
    output = *(this->output_);
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    FETCH_UNUSED(inputs);
    assert(inputs.empty());
    return {error_signal};
  }

  virtual bool SetData(ArrayType const &data)
  {
    bool shape_changed = true;
    if (this->output_)
    {
      shape_changed = (this->output_->shape() != data.shape());
    }
    this->output_ = std::make_shared<ArrayType>(data);
    return shape_changed;
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    FETCH_UNUSED(inputs);
    return this->output_->shape();
  }

  static constexpr char const *DESCRIPTOR = "PlaceHolder";

protected:
  ArrayPtrType output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
