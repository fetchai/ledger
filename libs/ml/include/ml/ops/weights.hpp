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

#include "ml/ops/placeholder.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Trainable
{
public:
  virtual void Step(T learningRate) = 0;
};

template <class T>
class Weights : public fetch::ml::ops::PlaceHolder<T>, public Trainable<typename T::Type>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Weights()          = default;
  virtual ~Weights() = default;

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    ASSERT(inputs.empty());
    gradientAccumulation_->InlineAdd(*errorSignal);
    return {};
  }

  virtual void SetData(ArrayPtrType const &data)
  {
    PlaceHolder<T>::SetData(data);
    if (this->output_ &&
        (!gradientAccumulation_ || gradientAccumulation_->shape() != this->output_->shape()))
    {
      gradientAccumulation_ = std::make_shared<ArrayType>(this->output_->shape());
    }
  }

  virtual void Step(typename T::Type learningRate)
  {
    for (std::size_t i(0); i < this->gradientAccumulation_->size(); ++i)
    {
      this->gradientAccumulation_->At(i) = this->gradientAccumulation_->At(i) * -learningRate;
    }
    this->output_->InlineAdd(*gradientAccumulation_);
    // Major DL framework do not do that, but as I can't think of any reason why, I'll leave it here
    // for convenience. Remove if needed -- Pierre
    gradientAccumulation_->Fill(typename T::Type(0));
  }

private:
  ArrayPtrType gradientAccumulation_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
