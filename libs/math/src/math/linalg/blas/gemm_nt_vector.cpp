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

#include "math/linalg/blas/gemm_nt_vector.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S, typename MATRIX>
void Blas<S, MATRIX, Signature(_C <= _alpha, _A, _B, _beta, _C),
          Computes(_C = _alpha * _A * T(_B) + _beta * _C), platform::Parallelisation::VECTORISE>::
     operator()(type const &alpha, MATRIX const &a, MATRIX const &b, type const &beta, MATRIX &c) const
{
  std::size_t j;
  if ((c.height() == 0) ||
      ((c.width() == 0) || (((alpha == 0.0) || (a.width() == 0)) && (beta == 1.0))))
  {
    return;
  }

  if (alpha == 0.0)
  {
    if (beta == 0.0)
    {
      for (j = 0; j < c.width(); ++j)
      {

        typename MATRIX::vector_register_type vec_zero(0.0);

        auto                 ret_slice = c.data().slice(c.padded_height() * j, c.height());
        memory::TrivialRange range(0, c.height());
        ret_slice.in_parallel().Apply(
            range,
            [vec_zero](typename MATRIX::vector_register_type &vw_c_j) { vw_c_j = vec_zero; });
      }
    }
    else
    {
      for (j = 0; j < c.width(); ++j)
      {

        typename MATRIX::vector_register_type vec_beta(beta);

        auto                 ret_slice = c.data().slice(c.padded_height() * j, c.height());
        auto                 slice_c_j = c.data().slice(c.padded_height() * j, c.height());
        memory::TrivialRange range(0, c.height());
        ret_slice.in_parallel().Apply(
            range,
            [vec_beta](typename MATRIX::vector_register_type const &vr_c_j,
                       typename MATRIX::vector_register_type &      vw_c_j) {
              vw_c_j = vec_beta * vr_c_j;
            },
            slice_c_j);
      }
    }

    return;
  }

  for (j = 0; j < c.width(); ++j)
  {
    std::size_t l;
    if (beta == 0.0)
    {

      typename MATRIX::vector_register_type vec_zero(0.0);

      auto                 ret_slice = c.data().slice(c.padded_height() * j, c.height());
      memory::TrivialRange range(0, c.height());
      ret_slice.in_parallel().Apply(
          range, [vec_zero](typename MATRIX::vector_register_type &vw_c_j) { vw_c_j = vec_zero; });
    }
    else if (beta != 1.0)
    {

      typename MATRIX::vector_register_type vec_beta(beta);

      auto                 ret_slice = c.data().slice(c.padded_height() * j, c.height());
      auto                 slice_c_j = c.data().slice(c.padded_height() * j, c.height());
      memory::TrivialRange range(0, c.height());
      ret_slice.in_parallel().Apply(
          range,
          [vec_beta](typename MATRIX::vector_register_type const &vr_c_j,
                     typename MATRIX::vector_register_type &vw_c_j) { vw_c_j = vec_beta * vr_c_j; },
          slice_c_j);
    }

    for (l = 0; l < a.width(); ++l)
    {
      type temp;
      temp = alpha * b(j, l);

      typename MATRIX::vector_register_type vec_temp(temp);

      auto                 ret_slice = c.data().slice(c.padded_height() * j, c.height());
      auto                 slice_c_j = c.data().slice(c.padded_height() * j, c.height());
      auto                 slice_a_l = a.data().slice(a.padded_height() * l, c.height());
      memory::TrivialRange range(0, c.height());
      ret_slice.in_parallel().Apply(range,
                                    [vec_temp](typename MATRIX::vector_register_type const &vr_c_j,
                                               typename MATRIX::vector_register_type const &vr_a_l,
                                               typename MATRIX::vector_register_type &vw_c_j) {
                                      vw_c_j = vr_c_j + vec_temp * vr_a_l;
                                    },
                                    slice_c_j, slice_a_l);
    }
  }
  return;
}

template class Blas<
    double,
    Matrix<double, fetch::memory::SharedArray<double>,
           fetch::math::RectangularArray<double, fetch::memory::SharedArray<double>, true, false>>,
    Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
    platform::Parallelisation::VECTORISE>;

template class Blas<
    float,
    Matrix<float, fetch::memory::SharedArray<float>,
           fetch::math::RectangularArray<float, fetch::memory::SharedArray<float>, true, false>>,
    Signature(_C <= _alpha, _A, _B, _beta, _C), Computes(_C = _alpha * _A * T(_B) + _beta * _C),
    platform::Parallelisation::VECTORISE>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch