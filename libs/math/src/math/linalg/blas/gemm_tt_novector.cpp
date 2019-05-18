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

#include "math/linalg/blas/gemm_tt_novector.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor_view.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S>
void Blas<S, Signature(_C <= _alpha, _A, _B, _beta, _C),
          Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C),
          platform::Parallelisation::NOT_PARALLEL>::operator()(Type const             alpha,
                                                               TensorView<Type> const a,
                                                               TensorView<Type> const b,
                                                               Type const             beta,
                                                               TensorView<Type>       c) const
{
  std::size_t i;
  std::size_t j;
  if ((c.height() == 0) ||
      ((c.width() == 0) || (((alpha == 0.0) || (a.height() == 0)) && (beta == 1.0))))
  {
    return;
  }

  if (alpha == 0.0)
  {
    if (beta == 0.0)
    {
      for (j = 0; j < c.width(); ++j)
      {
        for (i = 0; i < c.height(); ++i)
        {
          c(i, j) = 0.0;
        }
      }
    }
    else
    {
      for (j = 0; j < c.width(); ++j)
      {
        for (i = 0; i < c.height(); ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }
    }

    return;
  }

  for (j = 0; j < c.width(); ++j)
  {
    for (i = 0; i < c.height(); ++i)
    {
      Type        temp;
      std::size_t l;
      temp = 0.0;
      for (l = 0; l < a.height(); ++l)
      {
        temp = temp + a(l, i) * b(j, l);
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

template class Blas<double, Signature(_C <= _alpha, _A, _B, _beta, _C),
                    Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C),
                    platform::Parallelisation::NOT_PARALLEL>;

template class Blas<float, Signature(_C <= _alpha, _A, _B, _beta, _C),
                    Computes(_C <= _alpha * T(_A) * T(_B) + _beta * _C),
                    platform::Parallelisation::NOT_PARALLEL>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch