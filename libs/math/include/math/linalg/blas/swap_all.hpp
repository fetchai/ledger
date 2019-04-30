#pragma once

/* The class defined in this file implements the equivalent of 
 * following Python code:
 *
 * import numpy as np
 * import copy
 *
 * def swap_all(n, x, m, y, p):
 *   A = copy.copy(x)    
 *   if p >= 0 and m >= 0:
 *     x[:(m * n):m] = y[:(p * n):p]
 *     y[:(p * n):p] = A[:(m * n):m]    
 *   elif p >= 0 and m < 0:        
 *     x[:(m * n + m):m] = y[:(p * n):p]
 *     y[:(p * n):p] = A[:(m * n + m):m] 
 *   elif p < 0 and m >= 0:
 *     x[:(m * n):m] = y[:(p * n + p):p]
 *     y[:(p * n + p):p] = A[:(m * n):m]
 *   else:
 *     x[m *(-n + 1)::m] = y[p *(-n + 1)::p]
 *     y[p *(-n + 1)::p] = A[m *(-n + 1)::m]    
 *   
 *   return x, y
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
            Signature( _x, _y <= _n, _x, _m, _y, _p ),
            Computes( _x, _y = _y, _x ), 
            V>
{
public:
  using type = S;
  using VectorRegisterType = typename Tensor< type >::VectorRegisterType;
  
  void operator()(int const &n, Tensor< type > &dx, int const &incx, Tensor< type > &dy, int const &incy ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch