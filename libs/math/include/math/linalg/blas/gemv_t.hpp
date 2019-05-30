#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------


/* The class defined in this file implements the equivalent of 
 * following Python code:
 *
 * import numpy as np
 * import copy
 *
 * def gemv_t(alpha, A, x, n, beta, y, m):
 *   leny = A.T.shape[0]
 *   lenx = A.T.shape[1]      
 *   if m >= 0 and n >= 0:
 *     y[::m] = alpha * np.dot(A.T, x[::n]) + beta * y[::m]
 *   elif m < 0 and n >= 0:
 *     y[-(leny -1)*m::m] = alpha * np.dot(A.T, x[::n]) + beta * y[-(leny -1)*m::m]
 *   elif m >= 0 and n < 0:
 *     y[::m] = alpha * np.dot(A.T, x[-(lenx -1)*n::n]) + beta * y[::m]
 *   else:
 *     y[-(leny -1)*m::m] = alpha * np.dot(A.T, x[-(lenx -1)*n::n]) + beta * y[-(leny -1)*m::m]
 *   
 *   return y
 *
 * Authors:
 *  - Fetch.AI Limited             (C++ version)
 *  - Univ. of Tennessee           (Fortran version)
 *  - Univ. of California Berkeley (Fortran version)
 *  - Univ. of Colorado Denver     (Fortran version)
 *  - NAG Ltd.                     (Fortran version)
 */

#include "math/linalg/prototype.hpp"
#include "math/tensor_view.hpp"
#include "math/linalg/blas/base.hpp"

namespace fetch
{
namespace math
{
namespace linalg 
{

template<typename S, uint64_t V>
class Blas< S, 
            Signature( _y <= _alpha, _A, _x, _n, _beta, _y, _m ),
            Computes( _y <= _alpha * T(_A) * _x + _beta * _y ), 
            V>
{
public:
  using Type = S;
  using VectorRegisterType = typename TensorView< Type >::VectorRegisterType;
  
  void operator()(Type const alpha, TensorView< Type > const a, TensorView< Type > const x, int const incx, Type const beta, TensorView< Type >y, int const incy ) const;
};

} // namespace linalg
} // namespace math
} // namepsace fetch