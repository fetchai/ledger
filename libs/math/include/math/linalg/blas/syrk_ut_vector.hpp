#pragma once

#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S>
class Blas<S, Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
           Computes(_C = _alpha * T(_A) * _A + _beta * _C), platform::Parallelisation::VECTORISE>
{
public:
  using vector_register_type = typename Matrix<S>::vector_register_type;
  using type                 = S;

  void operator()(double const &alpha, Matrix<double> const &a, double const &beta,
                  Matrix<double> &c)
  {
    std::size_t i;
    std::size_t j;
    if ((c.height() == 0) || (((alpha == 0.0) || (a.height() == 0)) && (beta == 1.0)))
    {
      return;
    }

    if (alpha == 0.0)
    {
      if (beta == 0.0)
      {
        for (j = 0; j < c.height(); ++j)
        {

          vector_register_type vec_zero(0.0);

          auto ret_slice = c.data().slice(c.padded_height() * j, j + 1);
          ret_slice.in_parallel().Apply(
              [vec_zero](vector_register_type &vw_c_j) { vw_c_j = vec_zero; });
        }
      }
      else
      {
        for (j = 0; j < c.height(); ++j)
        {

          vector_register_type vec_beta(beta);

          auto ret_slice = c.data().slice(c.padded_height() * j, j + 1);
          auto slice_c_j = c.data().slice(c.padded_height() * j, j + 1);
          ret_slice.in_parallel().Apply(
              [vec_beta](vector_register_type const &vr_c_j, vector_register_type &vw_c_j) {
                vw_c_j = vec_beta * vr_c_j;
              },
              slice_c_j);
        }
      }

      return;
    }

    for (j = 0; j < c.height(); ++j)
    {
      for (i = 0; i < j + 1; ++i)
      {
        double temp;

        temp = 0.0;

        auto slice_a_i = a.data().slice(a.padded_height() * i, a.height());
        auto slice_a_j = a.data().slice(a.padded_height() * j, a.height());
        temp           = slice_a_i.in_parallel().SumReduce(
            [](vector_register_type const &vr_a_i, vector_register_type const &vr_a_j) {
              return vr_a_i * vr_a_j;
            },
            slice_a_j);
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
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch
