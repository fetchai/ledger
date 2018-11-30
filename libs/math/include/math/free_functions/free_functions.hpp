#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "math/kernels/approx_exp.hpp"
#include "math/kernels/approx_log.hpp"
#include "math/kernels/approx_logistic.hpp"
#include "math/kernels/approx_soft_max.hpp"
#include "math/kernels/basic_arithmetics.hpp"
#include "math/kernels/relu.hpp"
#include "math/kernels/sign.hpp"
#include "math/kernels/standard_functions.hpp"

#include "core/assert.hpp"
#include "math/ndarray_broadcast.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/memory/range.hpp"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <vector>

/////////////////////////////////
/// include specific math functions
/////////////////////////////////

#include "math/free_functions/fundamental_operators.hpp"  // add, subtract etc.
#include "math/free_functions/standard_functions/abs.hpp"
#include "math/free_functions/standard_functions/exp.hpp"
#include "math/free_functions/standard_functions/fmod.hpp"
#include "math/free_functions/standard_functions/log.hpp"
#include "math/free_functions/standard_functions/remainder.hpp"
#include "math/free_functions/statistics/normal.hpp"

/////////////////////////////////
/// blas libraries
/////////////////////////////////

#include "math/linalg/blas/gemm_nn_vector.hpp"
#include "math/linalg/blas/gemm_nn_vector_threaded.hpp"
#include "math/linalg/blas/gemm_nt_vector.hpp"
#include "math/linalg/blas/gemm_nt_vector_threaded.hpp"
#include "math/linalg/blas/gemm_tn_vector.hpp"
#include "math/linalg/blas/gemm_tn_vector_threaded.hpp"

#include "math/meta/type_traits.hpp"

#include "math/free_functions/exponentiation/exponentiation.hpp"
#include "math/free_functions/trigonometry/trigonometry.hpp"
#include "math/free_functions/comparison/comparison.hpp"
#include "math/free_functions/statistics/distributions.hpp"
#include "math/free_functions/precision/precision.hpp"
#include "math/free_functions/iteration/iteration.hpp"
#include "math/free_functions/deep_learning/activation_functions.hpp"
#include "math/free_functions/deep_learning/loss_functions.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/free_functions/sign/sign.cpp"
#include "math/free_functions/type/type.cpp"
#include "math/free_functions/numerical_decomposition/numerical_decomposition.cpp"

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;
template <typename T, typename C>
class NDArrayIterator;

namespace linalg {
template <typename T, typename C, typename S>
class Matrix;
}

template <typename T, typename C>
T Max(ShapeLessArray<T, C> const &array);

}  // namespace math
}  // namespace fetch
