#pragma once

/* The class defined in this file implements the equivalent of 
 * following Python code:
 *
 * import numpy as np
 * import copy
 *
 * def scal_all(n, alpha, x, m):
 *   if m == 1:
 *     x = alpha * x
 *   elif m > 0:
 *     x[:n*m:m] = alpha * x[:n*m:m]
 *   # else nothing to do for negative increases
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
            Signature( _x <= _n, _alpha, _x, _m ),
            Computes( _x = _alpha * _x ), 
            V>
{
public:
  using type = S;
  using VectorRegisterType = typename Tensor< type >::VectorRegisterType;
  
  void operator()(int const &n, type const &da, Tensor< type > &dx, int const &incx ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch