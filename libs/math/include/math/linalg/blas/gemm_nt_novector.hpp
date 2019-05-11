#pragma once

/* The class defined in this file implements the equivalent of 
 * following Python code:
 *
 * import numpy as np
 * import copy
 *
 * def gemm_nt_novector(alpha, A, B, beta, C):
 *   C = alpha * np.dot(A, B.T) + beta * C
 *   
 *   return C
 *
 * Authors:
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

template<typename S>
class Blas< S, 
            Signature( _C <= _alpha, _A, _B, _beta, _C ),
            Computes( _C <= _alpha * _A * T(_B) + _beta * _C ), 
            platform::Parallelisation::NOT_PARALLEL>
{
public:
  using type = S;
  using VectorRegisterType = typename Tensor< type >::VectorRegisterType;
  
  void operator()(type const &alpha, Tensor< type > const &a, Tensor< type > const &b, type const &beta, Tensor< type > &c ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch