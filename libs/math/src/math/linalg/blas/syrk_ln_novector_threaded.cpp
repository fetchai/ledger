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

#include "math/linalg/blas/syrk_ln_novector_threaded.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, typename MATRIX>
void Blas<S, MATRIX, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
          Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::THREADING>::
     operator()(type const &alpha, MATRIX const &a, type const &beta, MATRIX &c)
{
  std::size_t i;
  std::size_t j;
  if ((c.height() == 0) || (((alpha == 0.0) || (a.width() == 0)) && (beta == 1.0)))
  {
    return;
  }

  if (alpha == 0.0)
  {
    if (beta == 0.0)
    {
      for (j = 0; j < c.height(); ++j)
      {
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = 0.0;
        }
      }
    }
    else
    {
      for (j = 0; j < c.height(); ++j)
      {
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }
    }

    return;
  }

  for (j = 0; j < c.height(); ++j)
  {
    pool_.Dispatch([j, alpha, a, beta, &c]() {
      std::size_t i;
      std::size_t l;
      if (beta == 0.0)
      {
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = 0.0;
        }
      }
      else if (beta != 1.0)
      {
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }

      for (l = 0; l < a.width(); ++l)
      {
        if (a(j, l) != 0.0)
        {
          type temp;
          temp = alpha * a(j, l);
          for (i = j + 1 - 1; i < c.height(); ++i)
          {
            c(i, j) = c(i, j) + temp * a(i, l);
          }
        }
      }
    });

    pool_.Wait();
  }
  return;
}

template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
    Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::THREADING>;

template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
    Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch