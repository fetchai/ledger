#pragma once

#include <algorithm>
#include <limits.h>
#include <limits>
#include <unistd.h>

namespace fetch {
namespace swarm {

// TODO(EJF):  We should update this class to use the new C++11 <random> library
class SwarmRandom
{
public:
  SwarmRandom(SwarmRandom &rhs)  = delete;
  SwarmRandom(SwarmRandom &&rhs) = delete;
  SwarmRandom operator=(SwarmRandom &rhs) = delete;
  SwarmRandom operator=(SwarmRandom &&rhs) = delete;

  SwarmRandom()  = default;
  ~SwarmRandom() = default;

  explicit SwarmRandom(uint32_t index)
  {
    auto seed = time(NULL) * index * index;
    srand(uint(seed));
  }

  int r(int lo, int hi) { return rand() % (hi - lo) + lo; }

  template <class T>
  T r(const T &hi)
  {
    T foo = T(rand());
    foo %= hi;
    return foo;
  }

  template <class C>
  const typename C::value_type &pickOne(const C &c)
  {
    auto pos = r(c.size());
    for (const typename C::value_type &value : c)
    {
      pos--;
      if (pos <= 0)
      {
        return value;
      }
    }
    return c.front();
  }

  template <class C>
  const typename C::value_type &pickOneWeighted(
      const C &c, std::function<double(const typename C::value_type &v)> weightFunction)
  {
    double total = 0;
    for (const typename C::value_type &value : c)
    {
      double w = weightFunction(value);
      total += std::max(0.0, w);
    }
    double choice = double(r(0, INT_MAX)) / double(INT_MAX);

    for (const typename C::value_type &value : c)
    {
      double w = weightFunction(value);
      choice -= std::max(0.0, w);
      if (w <= 0)
      {
        return value;
      }
    }
    return c.front();
  }

  template <class C>
  const typename C::value_type &pickOne(const C &c, size_t maximum)
  {
    auto                       pos = r(std::max(c.size(), maximum));
    typename C::const_iterator ci  = c.begin();
    while (pos > 0)
    {
      ++ci;
      pos--;
    }
    return *ci;
  }
};

}  // namespace swarm
}  // namespace fetch
