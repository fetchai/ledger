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

#include "math/linalg/blas/swap_all.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor_view.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, uint64_t V>
void Blas<S, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x), V>::operator()(
    int const n, TensorView<Type> dx, int const incx, TensorView<Type> dy, int const incy) const
{
  Type dtemp;
  int  i;
  if ((incx == 1) && (incy == 1))
  {
    int m;
    int mp1;
    m = n % 3;
    if (m != 0)
    {
      for (i = 0; i < m; ++i)
      {
        dtemp = dx[i];
        dx[i] = dy[i];
        dy[i] = dtemp;
      }

      if (n < 3)
      {
        return;
      }
    }

    mp1 = 1 + m;
    for (i = mp1 - 1; i < n; i += 3)
    {
      dtemp     = dx[i];
      dx[i]     = dy[i];
      dy[i]     = dtemp;
      dtemp     = dx[1 + i];
      dx[1 + i] = dy[1 + i];
      dy[1 + i] = dtemp;
      dtemp     = dx[2 + i];
      dx[2 + i] = dy[2 + i];
      dy[2 + i] = dtemp;
    }
  }
  else
  {
    int ix;
    int iy;
    ix = 0;
    iy = 0;
    if (incx < 0)
    {
      ix = (1 + (-n)) * incx;
    }

    if (incy < 0)
    {
      iy = (1 + (-n)) * incy;
    }

    for (i = 0; i < n; ++i)
    {
      dtemp  = dx[ix];
      dx[ix] = dy[iy];
      dy[iy] = dtemp;
      ix     = ix + incx;
      iy     = iy + incy;
    }
  }

  return;
}

template class Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::NOT_PARALLEL>;
template class Blas<float, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::NOT_PARALLEL>;
template class Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::THREADING>;
template class Blas<float, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::THREADING>;
template class Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::VECTORISE>;
template class Blas<float, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::VECTORISE>;
template class Blas<double, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>;
template class Blas<float, Signature(_x, _y <= _n, _x, _m, _y, _p), Computes(_x, _y <= _y, _x),
                    platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch