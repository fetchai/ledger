#pragma once

namespace fetch {
namespace vectorize {


template< typename T, std::size_t S >
VectorRegister<T, S>  pow(VectorRegister<T, S> base, int p) 
{
  VectorRegister<T, S> result = VectorRegister<T, S>(1);
  if (p & 1)
    result *= base;
  p>>=1;
  
  while (p != 0)
  {
    base = base * base;
    if (p & 1)
      result *= base;
    p >>= 1;
  }

  return result;
}

}
}

