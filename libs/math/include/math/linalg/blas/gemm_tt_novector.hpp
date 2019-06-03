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
 * def gemm_tt_novector(alpha, A, B, beta, C):
 *   C = alpha * np.dot(A.T, B.T) + beta * C
 *
 *   return C
 *
 * Authors:
 */

#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor_view.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S>
class Blas<S, Signature(_C <= _alpha, _A, _B, _beta, _C),
           Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C),
           platform::Parallelisation::NOT_PARALLEL>
{
public:
  using Type               = S;
  using VectorRegisterType = typename TensorView<Type>::VectorRegisterType;

  void operator()(Type const alpha, TensorView<Type> const a, TensorView<Type> const b,
                  Type const beta, TensorView<Type> c) const;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch