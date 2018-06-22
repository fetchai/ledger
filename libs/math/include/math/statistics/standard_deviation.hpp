#ifndef MATH_STATISTICS_STANDARD_DEVIATION_HPP
#define MATH_STATISTICS_STANDARD_DEVIATION_HPP
#include"math/shape_less_array.hpp"
#include"math/statistics/variance.hpp"
#include"vectorise/memory/range.hpp"
#include"core/assert.hpp"

#include<cmath>

namespace fetch {
namespace math {
namespace statistics {

template< typename A >
inline typename A::type StandardDeviation(A const &a) 
{
  typedef typename A::type type;  

  type m = Variance( a );  
  return std::sqrt(m);  
}


}
}
}


#endif
