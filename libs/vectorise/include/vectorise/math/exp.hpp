#pragma once

namespace fetch {
namespace vectorize {


template< typename T, std::size_t S >
VectorRegister<T, S>  exp(VectorRegister<T, S> x, T const &precision = 0.00001) 
{
  VectorRegister<T, S> ret( T(0) ), xserie( T(1) );
  VectorRegister<T, S> p(precision);      
  std::size_t n = 0;
  
  while( any_less_than( p, abs(xserie) )  ) {
    ret = ret + xserie;
    ++n;
    
    VectorRegister<T, S> vecn( (T( n )) );    
    xserie = xserie * ( x / vecn );    
  }
    
  return ret;
}

}
}

