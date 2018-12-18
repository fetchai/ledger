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
#include "math/linalg/blas/gemv_n.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, typename MATRIX, uint64_t V>
void Blas<S, MATRIX, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
          Computes(_y = _alpha * _A * _x + _beta * _y), V>::
     operator()(type const &alpha, MATRIX const &a, ShapelessArray<type> const &x, int const &incx,
           type const &beta, ShapelessArray<type> &y, int const &incy) const
{
  int  iy;
  type temp;
  int  kx;
  int  j;
  int  jx;
  int  i;
  int  ky;
  int  leny;
  int  lenx;
  if ((int(a.height()) == 0) || ((int(a.width()) == 0) || ((alpha == 0.0) && (beta == 1.0))))
  {
    return;
  }

  lenx = int(a.width());
  leny = int(a.height());
  if (incx > 0)
  {
    kx = 1;
  }
  else
  {
    kx = 1 + (-(-1 + lenx) * incx);
  }

  if (incy > 0)
  {
    ky = 1;
  }
  else
  {
    ky = 1 + (-(-1 + leny) * incy);
  }

  if (beta != 1.0)
  {
    if (incy == 1)
    {
      if (beta == 0.0)
      {
        for (i = 0; i < leny; ++i)
        {
          y[i] = 0.0;
        }
      }
      else
      {
        for (i = 0; i < leny; ++i)
        {
          y[i] = beta * y[i];
        }
      }
    }
    else
    {
      iy = -1 + ky;
      if (beta == 0.0)
      {
        for (i = 0; i < leny; ++i)
        {
          y[iy] = 0.0;
          iy    = iy + incy;
        }
      }
      else
      {
        for (i = 0; i < leny; ++i)
        {
          y[iy] = beta * y[iy];
          iy    = iy + incy;
        }
      }
    }
  }

  if (alpha == 0.0)
  {
    return;
  }

  jx = -1 + kx;
  if (incy == 1)
  {
    for (j = 0; j < int(a.width()); ++j)
    {
      temp = alpha * x[jx];
      for (i = 0; i < int(a.height()); ++i)
      {
        y[i] = y[i] + temp * a(i, j);
      }

      jx = jx + incx;
    }
  }
  else
  {
    for (j = 0; j < int(a.width()); ++j)
    {
      temp = alpha * x[jx];
      iy   = -1 + ky;
      for (i = 0; i < int(a.height()); ++i)
      {
        y[iy] = y[iy] + temp * a(i, j);
        iy    = iy + incy;
      }

      jx = jx + incx;
    }
  }

  return;
}

template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * _A * _x + _beta * _y), platform::Parallelisation::NOT_PARALLEL>;
template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * _A * _x + _beta * _y), platform::Parallelisation::NOT_PARALLEL>;
template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * _A * _x + _beta * _y), platform::Parallelisation::THREADING>;
template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * _A * _x + _beta * _y), platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch
