#pragma once

#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S>
class Blas<S, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
           Computes(_C = _alpha * _A * T(_A) + _beta * _C),
           platform::Parallelisation::VECTORISE | platform::Parallelisation::THREADING>
{
public:
  using vector_register_type = typename Matrix<S>::vector_register_type;
  using type                 = S;

  void operator()(type const &alpha, Matrix<type> const &a, type const &beta, Matrix<type> &c)
  {
    std::size_t j;
    if ((c.height() == 0) || (((alpha == 0.0) || (a.width() == 0)) && (beta == 1.0)))
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

          auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply(
              [vec_zero](vector_register_type &vw_c_j) { vw_c_j = vec_zero; });
        }
      }
      else
      {
        for (j = 0; j < c.height(); ++j)
        {

          vector_register_type vec_beta(beta);

          auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
          auto slice_c_j = c.data().slice(c.padded_height() * j, c.height());
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
      pool_.Dispatch([j, alpha, a, beta, &c]() {
        std::size_t l;
        if (beta == 0.0)
        {

          vector_register_type vec_zero(0.0);

          auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply(
              [vec_zero](vector_register_type &vw_c_j) { vw_c_j = vec_zero; });
        }
        else if (beta != 1.0)
        {

          vector_register_type vec_beta(beta);

          auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
          auto slice_c_j = c.data().slice(c.padded_height() * j, c.height());
          ret_slice.in_parallel().Apply(
              [vec_beta](vector_register_type const &vr_c_j, vector_register_type &vw_c_j) {
                vw_c_j = vec_beta * vr_c_j;
              },
              slice_c_j);
        }

        for (l = 0; l < a.width(); ++l)
        {
          if (a(j, l) != 0.0)
          {
            double temp;
            temp = alpha * a(j, l);

            vector_register_type vec_temp(temp);

            auto ret_slice = c.data().slice(c.padded_height() * j, c.height());
            auto slice_c_j = c.data().slice(c.padded_height() * j, c.height());
            auto slice_a_l = a.data().slice(a.padded_height() * l, c.height());
            ret_slice.in_parallel().Apply(
                [vec_temp](vector_register_type const &vr_c_j, vector_register_type const &vr_a_l,
                           vector_register_type &vw_c_j) { vw_c_j = vr_c_j + vec_temp * vr_a_l; },
                slice_c_j, slice_a_l);
          }
        }
      });

      pool_.Wait();
    }
    return;
  }

private:
  threading::Pool pool_;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch