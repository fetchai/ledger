#pragma once

#include "math/tensor.hpp"
#include "meta/is_iterable.hpp"


namespace fetch
{
namespace math
{

template<typename T1, typename T2>
fetch::meta::IsIterableTwoArg<T1, T2, void> PolyfillInlineAdd(T1 &ret, T2 const& other)
{
  auto it1 = ret.begin();
  auto eit = ret.end();
  auto it2 = other.begin();

  while(it1.is_valid())
  {
    *it1 += it2;
    ++it1;
    ++it2;
  }
}

}
}