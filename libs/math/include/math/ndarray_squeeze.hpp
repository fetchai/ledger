#pragma once

#include "math/ndarray_iterator.hpp"
#include <cassert>
#include <unordered_set>

namespace fetch {
namespace math {

// need to forward declare
template <typename T, typename C>
class NDArray;

bool ShapeFromSqueeze(std::vector<std::size_t> const &a, std::vector<std::size_t> &b,
                      uint64_t const &axis = uint64_t(-1))
{
  std::size_t i = 0;
  b.clear();
  if (axis == uint64_t(-1))
  {
    for (auto const &v : a)
    {
      if (v != 1)
      {
        b.push_back(v);
      }
      ++i;
    }
  }
  else
  {
    for (auto const &v : a)
    {
      if (!((i == axis) && (v == 1)))
      {
        b.push_back(v);
      }
      ++i;
    }
  }

  return b.size() != a.size();
}

bool ShapeFromSqueeze(std::vector<std::size_t> const &a, std::vector<std::size_t> &b,
                      std::unordered_set<uint64_t> const &axes)
{
  std::size_t i = 0;
  b.clear();
  for (auto const &v : a)
  {
    if (!((axes.find(i) != axes.end()) && (v == 1)))
    {
      b.push_back(v);
    }
    ++i;
  }

  return b.size() != a.size();
}

template <typename T, typename C>
void Squeeze(NDArray<T, C> &arr, uint64_t const &axis = uint64_t(-1))
{
  std::vector<std::size_t> newshape;
  ShapeFromSqueeze(arr.shape(), newshape, axis);
  arr.Reshape(newshape);
}

template <typename T, typename C>
void Squeeze(NDArray<T, C> &arr, std::unordered_set<uint64_t> const &axes)
{
  std::vector<std::size_t> newshape;
  ShapeFromSqueeze(arr.shape(), newshape, axes);
  arr.Reshape(newshape);
}

template <typename F, typename T, typename C>
void Reduce(F fnc, NDArray<T, C> &input, NDArray<T, C> &output, T const &def_value = 0,
            uint64_t const &axis = 0)
{
  std::size_t N;

  std::size_t              k = 1;
  std::vector<std::size_t> out_shape;
  for (std::size_t i = 0; i < input.shape().size(); ++i)
  {
    if (i != axis)
    {
      out_shape.push_back(input.shape(i));
      k *= input.shape(i);
    }
  }
  output.Resize(k);
  output.Reshape(out_shape);

  NDArrayIterator<T, C> it_a(input);  // TODO(tfr): Make const iterator
  NDArrayIterator<T, C> it_b(output);

  if (axis != 0)
  {
    it_a.PermuteAxes(0, axis);
    it_b.PermuteAxes(0, axis);
  }

  N = it_a.range(0).total_steps;

  while (bool(it_a) && bool(it_b))
  {

    *it_b = def_value;
    for (std::size_t i = 0; i < N; ++i)
    {
      *it_b = fnc(*it_b, *it_a);
      ++it_a;
    }
    ++it_b;
  }
}

}  // namespace math
}  // namespace fetch