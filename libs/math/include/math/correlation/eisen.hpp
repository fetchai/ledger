#ifndef MATH_CORRELATION_EISEN_HPP
#define MATH_CORRELATION_EISEN_HPP
#include"math/shape_less_array.hpp"
#include"vectorise/memory/range.hpp"
#include"core/assert.hpp"

#include<cmath>

namespace fetch {
namespace math {
namespace correlation {

  template< typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE >
inline typename memory::VectorSlice<T,S>::type Eisen(memory::VectorSlice<T,S> const &a, memory::VectorSlice<T,S> const &b) 
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T,S>::vector_register_type vector_register_type;
  typedef typename memory::VectorSlice<T,S>::type type;  

  type innerA = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()), [](vector_register_type const &x) {
      return x * x;
    });  
  
  type innerB = b.in_parallel().SumReduce(memory::TrivialRange(0, b.size()), [](vector_register_type const &x) {
      return x * x;
    });

  type top = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()),[](vector_register_type const &x, vector_register_type const &y) {
      return x * y;
    }, b );

  type denom = type(sqrt(innerA * innerB));
  
  if(top < 0) top = - top;  
  return type(top / denom);
}

template< typename T, typename C >
inline typename ShapeLessArray<T,C>::type Eisen( ShapeLessArray<T,C> const &a,  ShapeLessArray<T,C> const &b) 
{
  return Eisen( a.data(), b.data());
}
  

}
}
}


#endif
