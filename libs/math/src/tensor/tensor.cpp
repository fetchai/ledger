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

////////////////////
/// SHAPE & SIZE ///
////////////////////

/**
 * returns the tensor's current shape
 * @return the stride of the tensor as a vector of size_type
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeVector const &Tensor<T, C>::stride() const
{
  return stride_;
}

/**
 * returns the tensor's current shape
 * @return  shape_ is the shape of the tensor as a vector of size_type
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeVector const &Tensor<T, C>::shape() const
{
  return shape_;
}

/**
 * returns the size of a specified dimension
 * @tparam T Type
 * @tparam C Container
 * @param n the dimension to query
 * @return SizeType value indicating the size of a dimension of the Tensor
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType const &Tensor<T, C>::shape(SizeType const &n) const
{
  return shape_[n];
}

/**
 * returns the size of the tensor
 * @tparam T Type
 * @tparam C Container
 * @return SizeType value indicating total size of Tensor
 */
template <typename T, typename C>
typename Tensor<T, C>::SizeType Tensor<T, C>::size() const
{
  return size_;
}

template class Tensor<std::int16_t>;
template class Tensor<std::int32_t>;
template class Tensor<std::int64_t>;
template class Tensor<std::uint16_t>;
template class Tensor<std::uint32_t>;
template class Tensor<std::uint64_t>;
template class Tensor<float>;
template class Tensor<double>;
template class Tensor<fixed_point::fp32_t>;
template class Tensor<fixed_point::fp64_t>;
template class Tensor<fixed_point::fp128_t>;

}  // namespace math
}  // namespace fetch
