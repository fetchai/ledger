#pragma once

/* The class defined in this file implements the equivalent of 
 * following Python code:
 *
 * import numpy as np
 * import copy
 *
 * def trmv_n(A, x, n):
 *   leny = A.shape[0]
 *   lenx = A.shape[1]      
 *   
 *   return x
 *
 * Authors:
 *  - Fetch.AI Limited             (C++ version)
 *  - Univ. of Tennessee           (Fortran version)
 *  - Univ. of California Berkeley (Fortran version)
 *  - Univ. of Colorado Denver     (Fortran version)
 *  - NAG Ltd.                     (Fortran version)
 */

#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
#include "math/linalg/blas/base.hpp"

namespace fetch
{
namespace math
{
namespace linalg 
{

template<typename S, uint64_t V>
class Blas< S, 
            Signature( _x <= _A, _x, _n ),
            Computes( _x = _A * _x ), 
            V>
{
public:
  using type = S;
  using VectorRegisterType = typename Tensor< type >::VectorRegisterType;
  
  void operator()(Tensor< type > const &a, Tensor< type > &x, int const &incx ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch