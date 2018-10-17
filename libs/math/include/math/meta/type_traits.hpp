#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

// TODO (private 179)


#include "core/byte_array/byte_array.hpp"
#include <type_traits>
//#include "math/shape_less_array.hpp"
//#include "math/rectangular_array.hpp"
//#include "math/linalg/matrix.hpp"

namespace fetch {
namespace math {
namespace meta {

template <typename T, typename C>
class ShapeLessArray;

template <typename T, typename C>
class RectangularArray;

// template <typename T, typename C>
// class NDArrayIterator;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename A, typename R>
struct IsBlasArrayLikeImpl
{
};

template <typename R>
struct IsBlasArrayLikeImpl<double, R>
{
  using Type = R;
};

// template<typename R, typename T, typename C, typename S>
// struct IsBlasArrayLikeImpl<linalg::Matrix<T, C, S>, R> {
//  using Type = R;
//};
//
// template<typename R, typename T, typename C>
// struct IsBlasArrayLikeImpl<RectangularArray<T, C>, R> {
//  using Type = R;
//};

template <typename R, typename T, typename C>
struct IsBlasArrayLikeImpl<ShapeLessArray<T, C>, R>
{
  using Type = R;
};






template <typename A, typename R>
struct IsBlasAndShapedArrayLikeImpl
{
};
template <typename R, typename T, typename C, typename S>
struct IsBlasAndShapedArrayLikeImpl<linalg::Matrix<T, C, S>, R>
{
  using Type = R;
};


// template< typename R, typename T, typename C>
// struct IsMathArrayLikeImpl< fetch::math::NDArray<T, C>, R>
//{
//  using Type = R;
//};

template <typename A, typename R>
using IsBlasArrayLike = typename IsBlasArrayLikeImpl<A, R>::Type;

template <typename A, typename R>
using IsBlasAndShapedArrayLike = typename IsBlasAndShapedArrayLikeImpl<A, R>::Type;

//
// template< typename T >
// IsMathArrayLike<T, bool> IsGreat(T const& x)
//{
//  return true;
//}

}  // namespace meta
}  // namespace math
}  // namespace fetch
