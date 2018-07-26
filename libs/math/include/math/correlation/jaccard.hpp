#pragma once
#include"math/shape_less_array.hpp"
#include"vectorise/memory/range.hpp"
#include"core/assert.hpp"

#include<cmath>

namespace fetch {
namespace math {
namespace correlation {

template< typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE >
inline typename memory::VectorSlice<T,S>::type Jaccard(memory::VectorSlice<T,S> const &a, memory::VectorSlice<T,S> const &b)   
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T,S>::vector_register_type vector_register_type;
  typedef typename memory::VectorSlice<T,S>::type type;  

  vector_register_type zero( type(0 ) );
  
  
  type sumA = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()), [zero](vector_register_type const &x, vector_register_type const &y) {
      return  min(x != zero,  y != zero);
    }, b);  

  
  type sumB = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()), [zero](vector_register_type const &x, vector_register_type const &y) {
      return max(x != zero,  y != zero);
    }, b);

  
  return type(sumA /(sumB));
}


template< typename T, typename C >
inline typename ShapeLessArray<T,C>::type Jaccard( ShapeLessArray<T,C> const &a,  ShapeLessArray<T,C> const &b) 
{
  return Jaccard( a.data(), b.data());
}
  

template< typename T, std::size_t S = memory::VectorSlice<T>::E_TYPE_SIZE >
inline typename memory::VectorSlice<T,S>::type GeneralisedJaccard(memory::VectorSlice<T,S> const &a, memory::VectorSlice<T,S> const &b)   
{
  detailed_assert(a.size() == b.size());
  typedef typename memory::VectorSlice<T,S>::vector_register_type vector_register_type;
  typedef typename memory::VectorSlice<T,S>::type type;  

  type sumA = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()), [](vector_register_type const &x, vector_register_type const &y) {
      return min(x, y);
    }, b);  
  
  type sumB = a.in_parallel().SumReduce(memory::TrivialRange(0, a.size()), [](vector_register_type const &x, vector_register_type const &y) {
      return max(x, y);
    }, b);

  return type(sumA / sumB);
}

template< typename T, typename C >
inline typename ShapeLessArray<T,C>::type GeneralisedJaccard( ShapeLessArray<T,C> const &a,  ShapeLessArray<T,C> const &b) 
{
  return GeneralisedJaccard( a.data(), b.data());
}

}
}
}


