#pragma once
#include "core/assert.hpp"
#include "math/correlation/jaccard.hpp"
#include "math/shape_less_array.hpp"
#include "vectorise/memory/range.hpp"

#include <cmath>

namespace fetch {
namespace math {
namespace distance {

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type Jaccard(memory::VectorSlice<T, S> const &a,
                                                        memory::VectorSlice<T, S> const &b)
{
  typedef typename memory::VectorSlice<T, S>::type type;

  return type(1) - correlation::Jaccard(a, b);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type Jaccard(ShapeLessArray<T, C> const &a,
                                                   ShapeLessArray<T, C> const &b)
{
  return Jaccard(a.data(), b.data());
}

template <typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE>
inline typename memory::VectorSlice<T, S>::type GeneralisedJaccard(
    memory::VectorSlice<T, S> const &a, memory::VectorSlice<T, S> const &b)
{
  typedef typename memory::VectorSlice<T, S>::type type;
  return type(1) - correlation::GeneralisedJaccard(a, b);
}

template <typename T, typename C>
inline typename ShapeLessArray<T, C>::type GeneralisedJaccard(ShapeLessArray<T, C> const &a,
                                                              ShapeLessArray<T, C> const &b)
{
  return GeneralisedJaccard(a.data(), b.data());
}

}  // namespace distance
}  // namespace math
}  // namespace fetch
