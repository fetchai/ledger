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
#include "math/linalg/blas/gemv_n.hpp"
#include "math/linalg/prototype.hpp"
#include "math/tensor.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, uint64_t V>
void Blas<S, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
          Computes(_y <= _alpha * _A * _x + _beta * _y), V>::operator()(Type const &        alpha,
                                                                        Tensor<Type> const &a,
                                                                        Tensor<Type> const &x,
                                                                        int const &         incx,
                                                                        Type const &        beta,
                                                                        Tensor<Type> &      y,
                                                                        int const &incy) const
{
  Type temp;
  int  i;
  int  iy;
  int  j;
  int  jx;
  int  kx;
  int  ky;
  int  lenx;
  int  leny;
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

        VectorRegisterType vec_zero(0.0);

        auto                 ret_slice = y.data().slice(0, y.padded_size());
        memory::TrivialRange range(std::size_t(0), std::size_t(leny));
        ret_slice.in_parallel().Apply(
            range, [vec_zero](VectorRegisterType &vw_v_y) { vw_v_y = vec_zero; });
      }
      else
      {

        VectorRegisterType vec_beta(beta);

        auto                 ret_slice = y.data().slice(0, y.padded_size());
        auto                 slice_v_y = y.data().slice(0, y.padded_size());
        memory::TrivialRange range(std::size_t(0), std::size_t(leny));
        ret_slice.in_parallel().Apply(
            range,
            [vec_beta](VectorRegisterType const &vr_v_y, VectorRegisterType &vw_v_y) {
              vw_v_y = vec_beta * vr_v_y;
            },
            slice_v_y);
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

      VectorRegisterType vec_temp(temp);

      auto ret_slice = y.data().slice(0, y.padded_size());
      auto slice_v_y = y.data().slice(0, y.padded_size());
      auto slice_a_j = a.data().slice(a.padded_height() * std::size_t(j), a.padded_height());
      memory::TrivialRange range(std::size_t(0), std::size_t(int(a.height())));
      ret_slice.in_parallel().Apply(
          range,
          [vec_temp](VectorRegisterType const &vr_v_y, VectorRegisterType const &vr_a_j,
                     VectorRegisterType &vw_v_y) { vw_v_y = vr_v_y + vec_temp * vr_a_j; },
          slice_v_y, slice_a_j);
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

template class Blas<double, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * _A * _x + _beta * _y),
                    platform::Parallelisation::VECTORISE>;
template class Blas<float, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * _A * _x + _beta * _y),
                    platform::Parallelisation::VECTORISE>;
template class Blas<double, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * _A * _x + _beta * _y),
                    platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>;
template class Blas<float, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
                    Computes(_y <= _alpha * _A * _x + _beta * _y),
                    platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch
