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

// StateDict is an utility class to extract the network trainable parameter and serialize them for
// saving / sharing
template <class T>
struct StateDict
{
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  ArrayPtrType                        weights_;
  std::map<std::string, StateDict<T>> dict_;

  bool operator==(StateDict<T> const &o) const
  {
    return !((bool(weights_) ^ bool(o.weights_)) || (weights_ && (*weights_ != *(o.weights_))) ||
             (dict_ != o.dict_));
  }

  StateDict &Merge(StateDict const &o, float ratio = .5f)
  {
    assert(ratio >= 0.0f && ratio <= 1.0f);
    if (ratio > 0)
    {
      if (weights_)
      {
        weights_->InlineMultiply(typename ArrayType::Type(1.0f - ratio));
        weights_->InlineAdd(o.weights_->Copy().InlineMultiply(typename ArrayType::Type(ratio)));
      }
      for (auto &e : dict_)
      {
        e.second.Merge(o.dict_.at(e.first));
      }
    }
    return *this;
  }
};

// Trainaible provide an interface that needs to be matched by any ops that has trainable
// parameters, that's how the Graph class know with tensors to update during a gradient step
template <class T>
class Trainable
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  virtual void                Step(typename T::Type learningRate)                            = 0;
  virtual struct StateDict<T> StateDict() const                                              = 0;
  virtual void                LoadStateDict(struct fetch::ml::ops::StateDict<T> const &dict) = 0;
};

template <class T>
class Weights : public fetch::ml::ops::PlaceHolder<T>, public Trainable<T>
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
    for (std::uint64_t i(0); i < this->gradientAccumulation_->size(); ++i)
    {
      this->gradientAccumulation_->At(i) = this->gradientAccumulation_->At(i) * -learningRate;
    }
    this->output_->InlineAdd(*gradientAccumulation_);
    // Major DL framework do not do that, but as I can't think of any reason why, I'll leave it here
    // for convenience. Remove if needed -- Pierre
    gradientAccumulation_->Fill(typename T::Type(0));
  }

  virtual struct StateDict<T> StateDict() const
  {
    struct fetch::ml::ops::StateDict<T> d;
    d.weights_ = this->output_;
    return d;
  }

  virtual void
  LoadStateDict(struct fetch::ml::ops::StateDict<T> const &dict)
  {
    assert(dict.dict_.empty());
    SetData(dict.weights_);
  }

private:
  ArrayPtrType gradientAccumulation_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
