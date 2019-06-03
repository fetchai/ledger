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

#include "math/linalg/blas/base.hpp"
#include "math/linalg/blas/gemv_t.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor_view.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, uint64_t V>
void Blas<S, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
          Computes(_y <= _alpha * T(_A) * _x + _beta * _y), V>::operator()(Type const alpha,
                                                                           TensorView<Type> const a,
                                                                           TensorView<Type> const x,
                                                                           int const        incx,
                                                                           Type const       beta,
                                                                           TensorView<Type> y,
                                                                           int const incy) const
{
  Type temp;
  int  i;
  int  j;
  int  jy;
  int  kx;
  int  ky;
  int  lenx;
  int  leny;
  if ((int(a.height()) == 0) || ((int(a.width()) == 0) || ((alpha == static_cast<Type>(0.0)) &&
                                                           (beta == static_cast<Type>(1.0)))))
  {
    return;
  }

  lenx = int(a.height());
  leny = int(a.width());
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

  if (beta != static_cast<Type>(1.0))
  {
    if (incy == 1)
    {
      if (beta == static_cast<Type>(0.0))
      {
        for (i = 0; i < leny; ++i)
        {
          y[i] = static_cast<Type>(0.0);
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
      int iy;
      iy = -1 + ky;
      if (beta == static_cast<Type>(0.0))
      {
        for (i = 0; i < leny; ++i)
        {
          y[iy] = static_cast<Type>(0.0);
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

  if (alpha == static_cast<Type>(0.0))
  {
    return;
  }

  jy = -1 + ky;
  if (incx == 1)
  {
    for (j = 0; j < int(a.width()); ++j)
    {
      temp = static_cast<Type>(0.0);
      for (i = 0; i < int(a.height()); ++i)
      {
        temp = temp + a(i, j) * x[i];
      }

      y[jy] = y[jy] + alpha * temp;
      jy    = jy + incy;
    }
  }
  else
  {
    for (j = 0; j < int(a.width()); ++j)
    {
      int ix;
      temp = static_cast<Type>(0.0);
      ix   = -1 + kx;
      for (i = 0; i < int(a.height()); ++i)
      {
        temp = temp + a(i, j) * x[ix];
        ix   = ix + incx;
      }

      y[jy] = y[jy] + alpha * temp;
      jy    = jy + incy;
    }
  }

  return;
}

template class Blas<double, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * T(_A) * _x + _beta * _y),
                    platform::Parallelisation::NOT_PARALLEL>;
template class Blas<float, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * T(_A) * _x + _beta * _y),
                    platform::Parallelisation::NOT_PARALLEL>;
template class Blas<double, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * T(_A) * _x + _beta * _y),
                    platform::Parallelisation::THREADING>;
template class Blas<float, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * T(_A) * _x + _beta * _y),
                    platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch
