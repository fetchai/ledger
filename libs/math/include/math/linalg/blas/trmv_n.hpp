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

#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S, uint64_t V>
class Blas<S, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x), V>
{
public:
  using type               = S;
  using VectorRegisterType = typename Tensor<type>::VectorRegisterType;

  void operator()(Tensor<type> const &a, Tensor<type> &x, int const &incx) const;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch