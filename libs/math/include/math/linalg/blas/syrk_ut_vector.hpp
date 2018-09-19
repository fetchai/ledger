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

#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S, typename MATRIX>
class Blas<S, MATRIX, Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
           Computes(_C = _alpha * T(_A) * _A + _beta * _C), platform::Parallelisation::VECTORISE>
{
public:
  using type                 = S;

  void operator()(type const &alpha, MATRIX const &a, type const &beta,
                  MATRIX &c) const;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch