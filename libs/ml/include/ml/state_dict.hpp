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

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include <list>

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
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = fetch::math::SizeType;

  ArrayPtrType                        weights_;
  std::map<std::string, StateDict<T>> dict_;

  bool operator==(StateDict<T> const &o) const
  {
    return !((bool(weights_) ^ bool(o.weights_)) || (weights_ && (*weights_ != *(o.weights_))) ||
             (dict_ != o.dict_));
  }

  void InlineDivide(typename ArrayType::Type n)
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
      weights_ = std::make_shared<ArrayType>(o.weights_->shape());
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
    ret.InlineDivide(static_cast<typename ArrayType::Type>(stateDictList.size()));
    return ret;
  }

  /**
   * Used to merge a stateDict into another
   * @param o -- The stateDict to merge into this
   * @param ratio -- this = this * ration + o * (1 - ratio)
   */
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

  ////////////////////////////////
  /// Serialization operations ///
  ////////////////////////////////

  template <typename S>
  friend void Serialize(S &serializer, StateDict const &t)
  {
    if (t.weights_)
    {
      serializer << true;
      serializer << *(t.weights_.get());
    }
    else
    {
      serializer << false;
    }

    SizeType dictsize = t.dict_.size();
    serializer << dictsize;

    for (auto const &d : t.dict_)
    {
      serializer << d.first;
      Serialize(serializer, d.second);
    }
  }

  template <typename S>
  friend void Deserialize(S &serializer, StateDict &t)
  {
    SizeType    dictsize{0};
    std::string nodename{};
    StateDict   node_sd;
    bool        hasweights{};

    serializer >> hasweights;
    if (hasweights)
    {
      ArrayType weights;
      serializer >> weights;
      t.weights_ = std::make_shared<ArrayType>(weights);
    }
    serializer >> dictsize;

    for (SizeType i = 0; i < dictsize; i++)
    {
      serializer >> nodename;
      Deserialize(serializer, node_sd);
      t.dict_.emplace(std::make_pair(nodename, node_sd));
    }
  }
};

}  // namespace ml
}  // namespace fetch
