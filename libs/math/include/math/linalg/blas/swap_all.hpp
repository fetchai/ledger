#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor/tensor_view.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S, uint64_t V>
class Blas<S, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x), V>  // NOLINT
{
public:
  using Type               = S;
  using VectorRegisterType = typename TensorView<Type>::VectorRegisterType;

  void operator()(int n, TensorView<Type> dx, int incx, TensorView<Type> dy, int incy) const;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch
