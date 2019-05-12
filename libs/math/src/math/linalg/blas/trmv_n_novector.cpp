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
#include "math/linalg/blas/trmv_n.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, uint64_t V>
void Blas<S, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x), V>::operator()(
    Tensor<type> const &a, Tensor<type> &x, int const &incx) const
{
  type temp;
  int  i;
  int  j;
  int  kx;
  if (incx <= 0)
  {
    kx = 1 + (-(-1 + int(a.width())) * incx);
  }
  else if (incx != 1)
  {
    kx = 1;
  }

  if (incx == 1)
  {
    for (j = int(a.width()) - 1; j < 1; j += -1)
    {
      if (x[j] != 0.0)
      {
        temp = x[j];
        for (i = int(a.width()) - 1; i < j + 1 + 1; i += -1)
        {
          x[i] = x[i] + temp * a(i, j);
        }

        x[j] = x[j] * a(j, j);
      }
    }
  }
  else
  {
    int jx;
    kx = kx + (-1 + int(a.width())) * incx;
    jx = -1 + kx;
    for (j = int(a.width()) - 1; j < 1; j += -1)
    {
      if (x[jx] != 0.0)
      {
        int ix;
        temp = x[jx];
        ix   = -1 + kx;
        for (i = int(a.width()) - 1; i < j + 1 + 1; i += -1)
        {
          x[ix] = x[ix] + temp * a(i, j);
          ix    = ix + (-incx);
        }

        x[jx] = x[jx] * a(j, j);
      }

      jx = jx + (-incx);
    }
  }

  return;
};

template class Blas<double, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x),
                    platform::Parallelisation::NOT_PARALLEL>;
template class Blas<float, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x),
                    platform::Parallelisation::NOT_PARALLEL>;
template class Blas<double, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x),
                    platform::Parallelisation::THREADING>;
template class Blas<float, Signature(_x <= _A, _x, _n), Computes(_x <= _A * _x),
                    platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch