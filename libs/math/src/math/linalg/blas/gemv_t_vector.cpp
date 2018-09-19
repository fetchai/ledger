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
#include "math/linalg/blas/gemv_t.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, typename MATRIX, uint64_t V>
void Blas<S, MATRIX, Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
          Computes(_y = _alpha * T(_A) * _x + _beta * _y), V>::
     operator()(type const &alpha, MATRIX const &a, ShapeLessArray<type> const &x, int const &incx,
           type const &beta, ShapeLessArray<type> &y, int const &incy) const
{
  int  jy;
  type temp;
  int  kx;
  int  j;
  int  i;
  int  ky;
  int  leny;
  int  lenx;
  if ((int(a.height()) == 0) || ((int(a.width()) == 0) || ((alpha == 0.0) && (beta == 1.0))))
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

  if (beta != 1.0)
  {
    if (incy == 1)
    {
      if (beta == 0.0)
      {

        typename MATRIX::vector_register_type vec_zero(0.0);

        auto                 ret_slice = y.data().slice(0, y.padded_size());
        memory::TrivialRange range(std::size_t(0), std::size_t(leny));
        ret_slice.in_parallel().Apply(
            range,
            [vec_zero](typename MATRIX::vector_register_type &vw_v_y) { vw_v_y = vec_zero; });
      }
      else
      {

        typename MATRIX::vector_register_type vec_beta(beta);

        auto                 ret_slice = y.data().slice(0, y.padded_size());
        auto                 slice_v_y = y.data().slice(0, y.padded_size());
        memory::TrivialRange range(std::size_t(0), std::size_t(leny));
        ret_slice.in_parallel().Apply(
            range,
            [vec_beta](typename MATRIX::vector_register_type const &vr_v_y,
                       typename MATRIX::vector_register_type &      vw_v_y) {
              vw_v_y = vec_beta * vr_v_y;
            },
            slice_v_y);
      }
    }
    else
    {
      int iy;
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

  jy = -1 + ky;
  if (incx == 1)
  {
    for (j = 0; j < int(a.width()); ++j)
    {
      temp = 0.0;

      auto slice_a_j = a.data().slice(a.padded_height() * std::size_t(j), a.padded_height());
      auto slice_v_x = x.data().slice(0, x.padded_size());
      memory::TrivialRange range(std::size_t(0), std::size_t(int(a.height())));
      temp = slice_a_j.in_parallel().SumReduce(
          range,
          [](typename MATRIX::vector_register_type const &vr_a_j,
             typename MATRIX::vector_register_type const &vr_v_x) { return vr_a_j * vr_v_x; },
          slice_v_x);
      y[jy] = y[jy] + alpha * temp;
      jy    = jy + incy;
    }
  }
  else
  {
    for (j = 0; j < int(a.width()); ++j)
    {
      int ix;
      temp = 0.0;
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

template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * T(_A) * _x + _beta * _y), platform::Parallelisation::VECTORISE>;
template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * T(_A) * _x + _beta * _y), platform::Parallelisation::VECTORISE>;
template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * T(_A) * _x + _beta * _y),
    platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>;
template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(_y <= _alpha, _A, _x, _n, _beta, _y, _m),
    Computes(_y = _alpha * T(_A) * _x + _beta * _y),
    platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch
