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
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <>
class Blas<double, Signature((U(_C) <= _alpha, U(_A), _beta, U(_C))),
           Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
{
public:
  using vector_register_type = typename Matrix<double>::vector_register_type;

  void operator()(double const &alpha, Matrix<double> const &a, double const &beta,
                  Matrix<double> &c)
  {
    std::size_t i;
    std::size_t j;
    if ((a.height() == 0) || (((alpha == 0.0) || (a.width() == 0)) && (beta == 1.0)))
    {
      return;
    }

    if (alpha == 0.0)
    {
      if (beta == 0.0)
      {
        for (j = 0; j < a.height(); ++j)
        {
          for (i = 0; i < j; ++i)
          {
            c(i, j) = 0.0;
          }
        }
      }
      else
      {
        for (j = 0; j < a.height(); ++j)
        {
          for (i = 0; i < j; ++i)
          {
            c(i, j) = beta * c(i, j);
          }
        }
      }

      return;
    }

    for (j = 0; j < a.height(); ++j)
    {
      std::size_t l;
      if (beta == 0.0)
      {
        for (i = 0; i < j; ++i)
        {
          c(i, j) = 0.0;
        }
      }
      else if (beta != 1.0)
      {
        for (i = 0; i < j; ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }

      for (l = 0; l < a.width(); ++l)
      {
        if (a(j, l) != 0.0)
        {
          double temp;
          temp = alpha * a(j, l);
          for (i = 0; i < j; ++i)
          {
            c(i, j) = c(i, j) + temp * a(i, l);
          }
        }
      }
    }
    return;
  }
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch
