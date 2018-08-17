#pragma once

#include "math/ndarray_iterator.hpp"
#include <assert.h>

namespace fetch {
namespace math {

// need to forward declare
template <typename T, typename C>
class NDArray;

bool ShapeFromBroadcast(std::vector<std::size_t> const &a, std::vector<std::size_t> const &b,
                        std::vector<std::size_t> &c)
{
  c.resize(std::max(a.size(), b.size()));

  auto it1 = a.rbegin();
  auto it2 = b.rbegin();
  auto cit = c.rbegin();

  while ((it1 != a.rend()) && (it2 != b.rend()))
  {

    assert(cit != c.rend());
    if ((*it1) == (*it2))
    {
      (*cit) = *it1;
    }
    else
    {
      if (((*it1) != 1) && ((*it2) != 1))
      {
        return false;
      }

      (*cit) = std::max((*it1), (*it2));
    }

    ++it1;
    ++it2;
    ++cit;
  }

  while (it1 != a.rend())
  {
    assert(cit != c.rend());
    (*cit) = *it1;
    ++it1;
    ++cit;
  }

  while (it2 != b.rend())
  {
    assert(cit != c.rend());
    (*cit) = *it2;
    ++it2;
    ++cit;
  }

  return true;
}

template <typename T, typename C>
bool UpgradeIteratorFromBroadcast(std::vector<std::size_t> const &a,
                                  NDArrayIterator<T, C> &         iterator)
{
  iterator.is_valid_ = false;

  auto &b   = iterator.ranges_;
  auto  it1 = a.rbegin();
  auto  it2 = b.rbegin();

  while ((it1 != a.rend()) && (it2 != b.rend()))
  {
    if ((it2->total_steps) == 1)
    {
      it2->repeat_dimension = *it1;
    }
    else if ((*it1) != (it2->total_steps))
    {
      return false;
    }

    ++it1;
    ++it2;
  }

  std::size_t total_repeats = 1;
  while (it1 != a.rend())
  {
    total_repeats *= (*it1);
    ++it1;
  }

  iterator.total_runs_ = total_repeats;

  iterator.is_valid_ = true;

  return true;
}

template <typename F, typename T, typename C>
bool Broadcast(F function, NDArray<T, C> &a, NDArray<T, C> &b, NDArray<T, C> &c)
{
  std::vector<std::size_t> cshape;

  ShapeFromBroadcast(a.shape(), b.shape(), cshape);

  c.ResizeFromShape(cshape);

  std::vector<std::vector<std::size_t>> rangeA, rangeB, rangeC;
  for (auto &i : a.shape())
  {
    rangeA.push_back({0, i});
  }

  for (auto &i : b.shape())
  {
    rangeB.push_back({0, i});
  }

  for (auto &i : c.shape())
  {
    rangeC.push_back({0, i});
  }

  NDArrayIterator<T, C> it_a(a, rangeA);
  NDArrayIterator<T, C> it_b(b, rangeB);
  NDArrayIterator<T, C> it_c(c, rangeC);

  if (!UpgradeIteratorFromBroadcast(cshape, it_a))
  {
    std::cout << "Could not promote iterator A" << std::endl;
    return false;
  }
  if (!UpgradeIteratorFromBroadcast(cshape, it_b))
  {
    std::cout << "Could not promote iterator B" << std::endl;
    return false;
  }

  while (it_c)
  {
    (*it_c) = function(*it_a, *it_b);

    ++it_a;
    ++it_b;
    ++it_c;
  }

  return true;
}

}  // namespace math
}  // namespace fetch
