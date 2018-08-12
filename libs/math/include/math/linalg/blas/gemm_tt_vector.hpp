#pragma once

#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S>
class Blas<S, Signature(_C <= _alpha, _A, _B, _beta, _C),
           Computes(_C = _alpha * T(_A) * T(_B) + _beta * _C), platform::Parallelisation::VECTORISE>
{
public:
  using vector_register_type = typename Matrix<S>::vector_register_type;
  using type                 = S;

  void operator()(double const &alpha, Matrix<double> const &a, Matrix<double> const &b,
                  double const &beta, Matrix<double> &c)
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

          vector_register_type vec_zero(0.0);

          auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply(
              [vec_zero](vector_register_type &vw_c) { vw_c = vec_zero; });
        }
      }
      else
      {
        for (j = 0; j < c.width(); ++j)
        {

          vector_register_type vec_beta(beta);

          auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
          auto slice_c   = c.data().slice(c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply(
              [vec_beta](vector_register_type const &vr_c, vector_register_type &vw_c) {
                vw_c = vec_beta * vr_c;
              },
              slice_c);
        }
      }

      return;
    }

    for (j = 0; j < c.width(); ++j)
    {
      for (i = 0; i < c.height(); ++i)
      {
        double      temp;
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
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch