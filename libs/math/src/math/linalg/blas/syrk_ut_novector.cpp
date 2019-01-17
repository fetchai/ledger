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

#include "math/linalg/blas/syrk_ut_novector.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, typename MATRIX>
void Blas<S, MATRIX, Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
          Computes(_C = _alpha * T(_A) * _A + _beta * _C),
          platform::Parallelisation::NOT_PARALLEL>::operator()(type const &alpha, MATRIX const &a,
                                                               type const &beta, MATRIX &c) const
{
  std::size_t i;
  std::size_t j;
  if ((c.height() == 0) || (((alpha == 0.0) || (a.height() == 0)) && (beta == 1.0)))
  {
    return;
  }

  if (alpha == 0.0)
  {
    if (beta == 0.0)
    {
      for (j = 0; j < c.height(); ++j)
      {
        for (i = 0; i < j + 1; ++i)
        {
          c(i, j) = 0.0;
        }
      }
    }
    else
    {
      for (j = 0; j < c.height(); ++j)
      {
        for (i = 0; i < j + 1; ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }
    }

    return;
  }

  for (j = 0; j < c.height(); ++j)
  {
    for (i = 0; i < j + 1; ++i)
    {
      std::size_t l;
      type        temp;
      temp = 0.0;
      for (l = 0; l < a.height(); ++l)
      {
        temp = temp + a(l, i) * a(l, j);
      }

      if (beta == 0.0)
      {
        c(i, j) = alpha * temp;
      }
      else
      {
        c(i, j) = alpha * temp + beta * c(i, j);
      }
    }
  }
  return;
}

template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
    Computes(_C = _alpha * T(_A) * _A + _beta * _C), platform::Parallelisation::NOT_PARALLEL>;

template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
    Computes(_C = _alpha * T(_A) * _A + _beta * _C), platform::Parallelisation::NOT_PARALLEL>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch