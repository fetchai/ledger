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

#include "math/tensor/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace math {

template class Tensor<float>;
//template class Tensor<double>;
//template class Tensor<fixed_point::fp32_t>;
//template class Tensor<fixed_point::fp64_t>;
//template class Tensor<fixed_point::fp128_t>;

//template <typename T, typename C>
//SizeType Tensor<T, C>::size() const
//{
//  return size_;
//}

}  // namespace math
}  // namespace fetch
