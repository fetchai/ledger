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
#include "math/meta/math_type_traits.hpp"

namespace fetch {
namespace math {

namespace details_vectorisation {

// TODO: vectorisation implementations not yet called

/**
 * subtract a scalar from every value in the array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  ASSERT(array.data().size() == ret.data().size());

  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x - val; },
      array.data());
}


/**
 * Multiply array by another array within a range
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                                       memory::Range const &range, ArrayType &ret)
{
  ASSERT(obj1.shape() == obj2.shape());
  ASSERT(obj1.shape() == ret.shape());

  // TODO (private 516)
  ASSERT(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Multiply(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x * y; },
                                   obj1.data(), obj2.data());
  }
}


/**
 * subtract array from another array within a range
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &obj1, ArrayType const &obj2,
                                              memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  assert(range.is_undefined() || range.is_trivial());

  if (range.is_undefined())
  {
    Subtract(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x - y; },
                                   obj1.data(), obj2.data());
  }
}


/**
 * BLAS implementation of array & scalar multiplication
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                              ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x * val; },
      array.data());
}



/**
 * divide array by a scalar
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */
template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = x / val; },
      array.data());
}
/**
 * elementwise divide scalar by array element
 * @tparam T
 * @tparam C
 * @param scalar
 * @param array
 * @param ret
 */
template <typename ArrayType, typename T>
meta::IfIsMathArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  assert(array.size() == ret.size());
  typename ArrayType::vector_register_type val(scalar);

  ret.data().in_parallel().Apply(
      [val](typename ArrayType::vector_register_type const &x,
            typename ArrayType::vector_register_type &      z) { z = val / x; },
      array.data());
}


template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                            memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (obj1.size() == obj2.size())
  {
    if (range.is_undefined())
    {
      Divide(obj1, obj2, ret);
    }
    else
    {
      auto r = range.ToTrivialRange(ret.data().size());

      ret.data().in_parallel().Apply(r,
                                     [](typename ArrayType::vector_register_type const &x,
                                        typename ArrayType::vector_register_type const &y,
                                        typename ArrayType::vector_register_type &z) { z = x / y; },
                                     obj1.data(), obj2.data());
    }
  }
}

} // details_vectorisation


namespace details {

template <typename ArrayType>
::fetch::math::meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &    array1,
                                                        ArrayType const &    array2,
                                                        memory::Range const &range, ArrayType &ret)
{
  assert(array1.size() == array2.size());
  assert(array1.size() == ret.size());
  //  ret.Reshape(array1.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Add(array1, array2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x + y; },
                                   array1.data(), array2.data());
  }
}

template <typename ArrayType>
::fetch::math::meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &    array1,
                                                             ArrayType const &    array2,
                                                             memory::Range const &range)
{
  ArrayType ret{array1.size()};
  Add(array1, array2, range, ret);
  return ret;
}

template <typename ArrayType>
::fetch::math::meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1,
                                                             ArrayType const &obj2, ArrayType &ret)
{
  ASSERT(obj1.size() == obj2.size());
  ASSERT(ret.size() == obj2.size());
  for (std::size_t i = 0; i < obj1.size(); ++i)
  {
    ret[i] = obj1[i] * obj2[i];
  }
}

template <typename ArrayType, typename T>
::fetch::math::meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array1,
                                                             ArrayType const &array2,
                                                             ArrayType &      ret)
{
  ASSERT(array1.size() == array2.size());
  ASSERT(ret.size() == array2.size());
  for (std::size_t i = 0; i < array1.size(); ++i)
  {
    ret[i] = array1[i] - array2[i];
  }
}

}  // namespace details

/////////////////
/// ADDITIONS ///
/////////////////

//////////////////
/// INTERFACES ///
//////////////////

template <typename S>
meta::IfIsArithmetic<S, S> Add(S const &scalar1, S const &scalar2)
{
  S ret;
  Add(scalar1, scalar2, ret);
  return ret;
}

template <typename T, typename ArrayType, typename = std::enable_if_t<meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Add(array, scalar, ret);
  return ret;
}
template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Add(T const &scalar, ArrayType const &array)
{
  return Add(array, scalar);
}

template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Add(T const &scalar, ArrayType const &array, ArrayType &ret)
{
  ret = Add(array, scalar, ret);
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Add(ArrayType const &array1, ArrayType const &array2)
{
  assert(array1.shape() == array2.shape());
  ArrayType ret{array1.shape()};
  Add(array1, array2, ret);
  return ret;
}

///////////////////////
/// IMPLEMENTATIONS ///
///////////////////////

/**
 * Implementation for scalar addition. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Add(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 + scalar2;
}

template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray <ArrayType, void> Add(ArrayType const &array, T const &scalar, ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while(it1.is_valid())
  {
    *rit = (*it1) + scalar;
    ++it1;
    ++rit;
  }
}

/**
 * Adds two arrays together
 * @tparam T
 * @tparam C
 * @param array1
 * @param array2
 * @param ret
 */
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Add(ArrayType const &array1, ArrayType const &array2,
                                              ArrayType &ret)
{
  ASSERT(array1.shape() == array2.shape());
  ASSERT(array1.shape() == ret.shape());

  auto it1 = array1.cbegin();
  auto it2 = array2.cbegin();
  auto rit = ret.begin();
  while(it1.is_valid())
  {
    *rit = (*it1) + (*it2);
    ++it1;
    ++it2;
    ++rit;
  }
}

//////////////////////////
/// ADDITION OPERATORS ///
//////////////////////////

template <typename OtherType>
meta::IfIsMath<OtherType, void> operator+=(OtherType &left, OtherType const &right)
{
  Add(left, right, left);
}

///////////////////
/// SUBTRACTION ///
///////////////////

//////////////////////
/// IMPLEMENTATION ///
//////////////////////

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Subtract(T const &scalar, ArrayType const &array,
                                                   ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  ASSERT(array.shape() == ret.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while(it1.is_valid())
  {
    *rit = scalar - *it1;
    ++it1;
    ++rit;
  }
}

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array, T const &scalar,
                                                        ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while(it1.is_valid())
  {
    *rit = *it1 - scalar;
    ++it1;
    ++rit;
  }
}



template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Subtract(ArrayType const &array1, ArrayType const &array2,
                                                 ArrayType &ret)
{
  // TODO: Broadcast
  assert(array1.shape() == array2.shape());
  assert(array1.shape() == ret.shape());

  auto it1 = array1.cbegin();
  auto it2 = array2.cbegin();
  auto eit1 = array1.cend();
  auto rit = ret.begin();

  while(it1 != eit1)
  {
    *rit = (*it1) - (*it2);
    ++it1;
    ++it2;
    ++rit;
  }

  /*
  assert((array.size() == array2.size()) ||
         ((array.shape()[0] == array2.shape()[0]) &&
          ((array.shape()[1] == 1) || (array2.shape()[1] == 1))) ||
         ((array.shape()[1] == array2.shape()[1]) &&
          ((array.shape()[0] == 1) || (array2.shape()[0] == 1))));
  assert((array.size() == ret.size()) || (array2.size() == ret.size()));

  if (array.size() == array2.size())
  {
    for (std::size_t i = 0; i < ret.size(); ++i)
    {
      ret.Set(i, array.At(i) - array2.At(i));
    }
  }

  // matrix - vector subtraction (i.e. broadcasting)
  else if (array.shape()[0] == 1)
  {
    for (std::size_t i = 0; i < array2.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < array2.shape()[1]; ++j)
      {
        ret.Set({i, j}, array.At({0, j}) - array2.At({i, j}));
      }
    }
  }
  else if (array.shape()[1] == 1)
  {
    for (std::size_t i = 0; i < array2.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < array2.shape()[1]; ++j)
      {
        ret.Set({i, j}, array.At({i, 0}) - array2.At({i, j}));
      }
    }
  }
  else if (array2.shape()[0] == 1)
  {
    for (std::size_t i = 0; i < array2.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < array2.shape()[1]; ++j)
      {
        ret.Set({i, j}, array.At({i, j}) - array2.At({0, j}));
      }
    }
  }
  else if (array2.shape()[1] == 1)
  {
    for (std::size_t i = 0; i < array2.shape()[0]; ++i)
    {
      for (std::size_t j = 0; j < array2.shape()[1]; ++j)
      {
        ret.Set({i, j}, array.At({0, j}) - array2.At({i, 0}));
      }
    }
  }
  else
  {
    throw std::runtime_error("broadcast subtraction for tensors more than 2D not yet handled");
  }
  */
}


/////////////////
/// INTERFACE ///
/////////////////

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Subtract(scalar, array, ret);
  return ret;
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Subtract(array, scalar, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &    obj1,
                                                        ArrayType const &    obj2,
                                                        memory::Range const &range)
{
  ArrayType ret{obj1.shape()};
  Subtract(obj1, obj2, range, ret);
  return ret;
}
template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Subtract(ArrayType const &obj1,
                                                        ArrayType const &obj2)
{
  assert(obj1.size() == obj2.size());
  ArrayType ret{obj1.shape()};
  Subtract(obj1, obj2, ret);
  return ret;
}


///////////////////////////////////////////////////////
/// SUBTRACTIONS - SCALAR & SCALAR - NO FIXED POINT ///
///////////////////////////////////////////////////////

/**
 * Implementation for scalar subtraction. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Subtract(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 - scalar2;
}
template <typename S>
meta::IfIsArithmetic<S, S> Subtract(S const &scalar1, S const &scalar2)
{
  S ret;
  Subtract(scalar1, scalar2, ret);
  return ret;
}

////////////////
/// Multiply ///
////////////////

//////////////////////////////////
/// MULTIPLY - IMPLEMENTATIONS ///
//////////////////////////////////


template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &array, T const &scalar,
                                                 ArrayType &ret)
{
  ASSERT(array.size() == ret.size());
  auto it1 = array.cbegin();
  auto it2 = ret.begin();
  while(it1.is_valid())
  {
    *it2 = scalar * (*it1);
    ++it1;
    ++it2;
  }

}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                                   memory::Range const &range, ArrayType &ret)
{
  assert(obj1.size() == obj2.size());
  assert(obj1.size() == ret.size());

  // TODO (private 516)
  assert(range.is_trivial() || range.is_undefined());

  if (range.is_undefined())
  {
    Multiply(obj1, obj2, ret);
  }
  else
  {
    auto r = range.ToTrivialRange(ret.data().size());

    ret.data().in_parallel().Apply(r,
                                   [](typename ArrayType::vector_register_type const &x,
                                      typename ArrayType::vector_register_type const &y,
                                      typename ArrayType::vector_register_type &z) { z = x * y; },
                                   obj1.data(), obj2.data());
  }
}
/**
 * Implementation for scalar multiplication. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Multiply(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 * scalar2;
}

//////////////////
/// INTERFACES ///
//////////////////

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Multiply(T const &scalar, ArrayType const &array,
                                              ArrayType &ret)
{
  Multiply(array, scalar, ret);
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(T const &scalar, ArrayType const &array)
{
  return Multiply(array, scalar);
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Multiply(array, scalar, ret);
  return ret;
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Multiply(ArrayType const &obj1, ArrayType const &obj2,
                                                 ArrayType &ret)
{
  details::Multiply(obj1, obj2, ret);
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Multiply(ArrayType const &obj1,
                                                             ArrayType const &obj2)
{
  assert(obj1.shape() == obj2.shape());
  ArrayType ret{obj1.shape()};
  Multiply(obj1, obj2, ret);
  return ret;
}
template <typename S>
meta::IfIsArithmetic<S, S> Multiply(S const &scalar1, S const &scalar2)
{
  S ret;
  Multiply(scalar1, scalar2, ret);
  return ret;
}

//////////////
/// DIVIDE ///
//////////////

/////////////////
/// INTERFACE ///
/////////////////

template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(ArrayType const &array, T const &scalar)
{
  ArrayType ret{array.shape()};
  Divide(array, scalar, ret);
  return ret;
}

//////////////////////
/// IMPLEMENTATION ///
//////////////////////


template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &array, T const &scalar,
                                               ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  auto it1 = array.cbegin();
  auto rit = ret.begin();
  while(it1.is_valid())
  {
    *rit = (*it1) / scalar;
    ++it1;
    ++rit;
  }
}
template <typename ArrayType, typename T,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, void> Divide(T const &scalar, ArrayType const &array,
                                               ArrayType &ret)
{
  ASSERT(array.shape() == ret.shape());
  auto it = array.begin();
  auto it2 = ret.begin();
  while (it.is_valid())
  {
    *it2 = scalar / *it;
    ++it2;
    ++it;
  }
}

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(ArrayType const &obj1, ArrayType const &obj2)
{
  assert(obj1.shape() == obj2.shape());
  ArrayType ret{obj1.shape()};
  Divide(obj1, obj2, ret);
  return ret;
}

template <typename T, typename ArrayType,
          typename = std::enable_if_t<fetch::math::meta::IsArithmetic<T>>>
meta::IfIsMathArray<ArrayType, ArrayType> Divide(T const &scalar, ArrayType const &array)
{
  ArrayType ret{array.shape()};
  Divide(scalar, array, ret);
  return ret;
}

/**
 * subtract array from another array
 * @tparam T
 * @tparam C
 * @param array1
 * @param scalar
 * @param ret
 */

namespace details {

template <typename ArrayType>
void NaiveDivideArray(ArrayType const &array1, ArrayType const &array2, ArrayType &ret)
{
  ASSERT(array1.size() == array2.size());
  ASSERT(ret.size() == array2.size());
  for (std::size_t j = 0; j < array1.size(); ++j) {
    ret[j] = array1[j] / array2[j];
  }
}

}  // namespace details

template <typename ArrayType>
meta::IfIsMathArray<ArrayType, void> Divide(ArrayType const &obj1, ArrayType const &obj2,
                                               ArrayType &ret)
{
  assert(obj1.shape() == obj2.shape());
  assert(ret.shape() == obj2.shape());
  details::NaiveDivideArray(obj1, obj2, ret);
}


/**
 * Implementation for scalar division. Implementing this helps keeps a uniform interface
 * @tparam T
 * @param scalar1
 * @param scalar2
 * @param ret
 */
template <typename S>
meta::IfIsArithmetic<S, void> Divide(S const &scalar1, S const &scalar2, S &ret)
{
  ret = scalar1 / scalar2;
}

template <typename S>
meta::IfIsArithmetic<S, S> Divide(S const &scalar1, S const &scalar2)
{
  S ret;
  Divide(scalar1, scalar2, ret);
  return ret;
}

}  // namespace math
}  // namespace fetch