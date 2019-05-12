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

#include <gtest/gtest.h>

#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/trmv_n.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"

using namespace fetch;
using namespace fetch::math;
using namespace fetch::math::linalg;

TEST(blas_trmv, blas_trmv_n_novector1)
{

  Blas<double, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x),
       platform::Parallelisation::NOT_PARALLEL>
      trmv_n_novector;
  // Compuing _x <= _A * _x
  using type = double;

  int n = 1;

  Tensor<type> A = Tensor<type>::FromString(R"(
  	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
  	)");

  Tensor<type> x = Tensor<type>::FromString(R"(
    0.05808361216819946; 0.8661761457749352
    )");

  trmv_n_novector(A, x, n);

  Tensor<type> refx = Tensor<type>::FromString(R"(
  0.05808361216819946; 0.8661761457749352
  )");

  ASSERT_TRUE(refx.AllClose(x));
}
