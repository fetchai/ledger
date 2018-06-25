#ifndef MATH_DISTANCE_PAIRWISE_DISTANCE_HPP
#define MATH_DISTANCE_PAIRWISE_DISTANCE_HPP
#include"math/shape_less_array.hpp"
#include"vectorise/memory/range.hpp"
#include"core/assert.hpp"

#include<cmath>


namespace fetch {
namespace math {
namespace distance {

  template<  typename A, typename F >
  inline A& PairWiseDistance(  A &r, A const &a, F && metric )
{
  detailed_assert(r.height()  == 1);
  detailed_assert(r.width() == (a.height() * (a.height() - 1 ) /  2));

  std::size_t offset1 = 0;
  std::size_t k = 0;
  
  for(std::size_t i=0; i < a.height(); ++i) {
    typename A::vector_slice_type slice1 = a.data().slice(offset1, a.width());
    offset1 += a.padded_width();    
    std::size_t offset2 = offset1;
    
    for(std::size_t j=i+1; j < a.height(); ++j) {    
      typename A::vector_slice_type slice2 = a.data().slice(offset2, a.width());    
      offset2 += a.padded_width();
      r(0,k++) = metric(slice1, slice2);
    }
  }
  
  return r;
}


}
}
}


#endif
