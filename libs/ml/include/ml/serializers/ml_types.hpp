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

#include "math/serializers/math_types.hpp"
#include "ml/ops/weights.hpp"

namespace fetch {
namespace serializers {

template <typename T, typename U>
inline void Serialize(T &serializer, struct ml::StateDict<U> const &sd)
{
  if (sd.weights_)
  {
    Serialize(serializer, true);
    Serialize(serializer, *sd.weights_);
  }
  else
  {
    Serialize(serializer, false);
  }

  if (!sd.dict_.empty())
  {
    Serialize(serializer, true);
    Serialize(serializer, sd.dict_);
  }
  else
  {
    Serialize(serializer, false);
  }
}

template <typename T, typename U>
inline void Deserialize(T &serializer, struct ml::StateDict<U> &sd)
{
  bool hasWeight;
  bool hasDict;

  Deserialize(serializer, hasWeight);
  if (hasWeight)
  {
    std::shared_ptr<U> w = std::make_shared<U>();
    Deserialize(serializer, *w);
    sd.weights_ = w;
  }

  Deserialize(serializer, hasDict);
  if (hasDict)
  {
    Deserialize(serializer, sd.dict_);
  }
}

}  // namespace serializers
}  // namespace fetch
