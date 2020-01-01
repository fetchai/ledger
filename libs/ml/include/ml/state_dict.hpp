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

#include "math/base_types.hpp"
#include "math/tensor/tensor.hpp"

#include <list>
#include <map>
#include <string>

namespace fetch {
namespace ml {

/**
 * A utility class to extract the network trainable parameter and serialize them for saving /
 * sharing
 * @tparam T
 */
template <class T>
struct StateDict
{
  using TensorType   = T;
  using DataType     = typename TensorType ::Type;
  using ArrayPtrType = std::shared_ptr<TensorType>;
  using SizeType     = fetch::math::SizeType;

  ArrayPtrType                        weights_;
  std::map<std::string, StateDict<T>> dict_;

  bool operator==(StateDict<T> const &o) const
  {
    return !((bool(weights_) ^ bool(o.weights_)) || (weights_ && (*weights_ != *(o.weights_))) ||
             (dict_ != o.dict_));
  }

  void InlineDivide(DataType n)
  {
    if (weights_)
    {
      weights_->InlineDivide(n);
    }
    for (auto &kvp : dict_)
    {
      kvp.second.InlineDivide(n);
    }
  }

  void InlineAdd(StateDict const &o, bool strict = true)
  {
    if (o.weights_ && !weights_ && !strict)
    {
      weights_ = std::make_shared<TensorType>(o.weights_->shape());
    }
    assert(!((bool(weights_) ^ bool(o.weights_))));
    if (weights_)
    {
      weights_->InlineAdd(*(o.weights_));
    }
    for (auto const &kvp : o.dict_)
    {
      dict_[kvp.first].InlineAdd(kvp.second, strict);
    }
  }

  /**
   * Used to merge a list of state dict together into a new object
   * All are weighted equally -- Usefull for averaging weights of multiple similar models
   * @param stateDictList
   */
  static StateDict MergeList(std::list<StateDict> const &stateDictList)
  {
    StateDict ret;
    for (auto const &sd : stateDictList)
    {
      ret.InlineAdd(sd, false);
    }
    ret.InlineDivide(static_cast<DataType>(stateDictList.size()));
    return ret;
  }

  /**
   * Used to merge a stateDict into another
   * @param o -- The stateDict to merge into this
   * @param ratio -- this = this * ration + o * (1 - ratio)
   */
  StateDict &Merge(StateDict const &o, DataType ratio = fetch::math::Type<DataType>("0.5"))
  {
    assert(ratio >= DataType{0} && ratio <= DataType{1});
    if (ratio > DataType{0})
    {
      if (weights_)
      {
        weights_->InlineMultiply(DataType{1} - ratio);
        weights_->InlineAdd(o.weights_->Copy().InlineMultiply(ratio));
      }
      for (auto &e : dict_)
      {
        e.second.Merge(o.dict_.at(e.first));
      }
    }
    return *this;
  }
};

}  // namespace ml
}  // namespace fetch
