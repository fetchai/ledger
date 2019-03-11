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
#include <memory>
#include <vector>

namespace fetch {
namespace math {

  template<typename T>
  std::shared_ptr<fetch::math::Tensor<T>> ConcatenateTensors(std::vector<std::shared_ptr<fetch::math::Tensor<T>>> const &tensors)
  {
    std::vector<typename fetch::math::Tensor<T>::SizeType> retSize;
    retSize.push_back(tensors.size());
    retSize.insert(retSize.back(), tensors.front()->shape().begin(), tensors.front()->shape().end());
    std::shared_ptr<fetch::math::Tensor<T>> ret = std::make_shared<fetch::math::Tensor<T>>(retSize);
    for (typename fetch::math::Tensor<T>::SizeType i(0) ; i < tensors.size() ; ++i)
      {
	ret->Slice(i).Copy(*(tensors[i]));
      }
    return ret;
  }

}  // namespace math
}  // namespace fetch
