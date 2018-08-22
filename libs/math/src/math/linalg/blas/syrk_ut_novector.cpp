#include "math/linalg/blas/syrk_ut_novector.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S>
void Blas<S, Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
          Computes(_C = _alpha * T(_A) * _A + _beta * _C),
          platform::Parallelisation::NOT_PARALLEL>::operator()(type const &        alpha,
                                                               Matrix<type> const &a,
                                                               type const &        beta,
                                                               Matrix<type> &      c) const
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
        for (i = 0; i < j + 1; ++i)
        {
          c(i, j) = 0.0;
        }
      }
    }
    else
    {
      for (j = 0; j < c.height(); ++j)
      {
        for (i = 0; i < j + 1; ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }
    }

    return;
  }

  for (j = 0; j < c.height(); ++j)
  {
    for (i = 0; i < j + 1; ++i)
    {
      std::size_t l;
      type        temp;
      temp = 0.0;
      for (l = 0; l < a.height(); ++l)
      {
        temp = temp + a(l, i) * a(l, j);
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

template class Blas<double, Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
                    Computes(_C = _alpha * T(_A) * _A + _beta * _C),
                    platform::Parallelisation::NOT_PARALLEL>;

template class Blas<float, Signature(U(_C) <= _alpha, U(_A), _beta, U(_C)),
                    Computes(_C = _alpha * T(_A) * _A + _beta * _C),
                    platform::Parallelisation::NOT_PARALLEL>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch