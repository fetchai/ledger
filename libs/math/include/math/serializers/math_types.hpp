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

#include "math/tensor.hpp"

namespace fetch {
namespace serializers {

template <typename T, typename U>
inline void Serialize(T &serializer, math::Tensor<U> const &t)
{
  Serialize(serializer, t.shape());
  Serialize(serializer, t.Strides());
  Serialize(serializer, t.Padding());
  Serialize(serializer, t.Offset());
  if (t.Storage())
  {
    Serialize(serializer, true);
    Serialize(serializer, *(t.Storage()));
  }
  else
  {
    Serialize(serializer, false);
  }
}

template <typename T, typename U>
inline void Deserialize(T &serializer, math::Tensor<U> &t)
{
  std::vector<typename math::Tensor<U>::SizeType> shape;
  std::vector<typename math::Tensor<U>::SizeType> strides;
  std::vector<typename math::Tensor<U>::SizeType> padding;
  typename math::Tensor<U>::SizeType              offset;
  bool                                            hasStorage;
  std::shared_ptr<std::vector<U>>                 storage;

  Deserialize(serializer, shape);
  Deserialize(serializer, strides);
  Deserialize(serializer, padding);
  Deserialize(serializer, offset);
  Deserialize(serializer, hasStorage);
  if (hasStorage)
  {
    storage = std::make_shared<std::vector<U>>();
    Deserialize(serializer, *storage);
  }
  t = math::Tensor<U>(shape, strides, padding, storage, offset);
}

}  // namespace serializers
}  // namespace fetch
