#pragma once
#include"math/shape_less_array.hpp"
#include"vectorise/memory/range.hpp"
#include"core/assert.hpp"

#include<cmath>

namespace fetch {
namespace math {
namespace distance {

template< typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE >
inline typename memory::VectorSlice<T,S>::type Euclidean(memory::VectorSlice<T,S> const &a, memory::VectorSlice<T,S> const &b)   
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T,S>::vector_register_type vector_register_type;
  typedef typename memory::VectorSlice<T,S>::type type;  
  
  type dist = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()), [](vector_register_type const &x,  vector_register_type const &y) {
      vector_register_type d = x - y;
      return d * d;
    }, b);
  
  return std::sqrt(dist);
}


  template< typename T, typename C >
  inline typename ShapeLessArray<T,C>::type Euclidean( ShapeLessArray<T,C> const &a,  ShapeLessArray<T,C> const &b) 
{
  return Euclidean( a.data(), b.data());
}
  

  

}
}
}


