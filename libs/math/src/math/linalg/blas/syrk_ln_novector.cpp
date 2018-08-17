#include "math/linalg/blas/syrk_ln_novector.hpp"
#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"
namespace fetch {
namespace math {
namespace linalg {

template <typename S>
void Blas<S, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
          Computes(_C = _alpha * _A * T(_A) + _beta * _C),
          platform::Parallelisation::NOT_PARALLEL>::operator()(type const &        alpha,
                                                               Matrix<type> const &a,
                                                               type const &        beta,
                                                               Matrix<type> &      c) const
{
  std::size_t i;
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
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = 0.0;
        }
      }
    }
    else
    {
      for (j = 0; j < c.height(); ++j)
      {
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = beta * c(i, j);
        }
      }
    }

    return;
  }

  for (j = 0; j < c.height(); ++j)
  {
    std::size_t l;
    if (beta == 0.0)
    {
      for (i = j + 1 - 1; i < c.height(); ++i)
      {
        c(i, j) = 0.0;
      }
    }
    else if (beta != 1.0)
    {
      for (i = j + 1 - 1; i < c.height(); ++i)
      {
        c(i, j) = beta * c(i, j);
      }
    }

    for (l = 0; l < a.width(); ++l)
    {
      if (a(j, l) != 0.0)
      {
        type temp;
        temp = alpha * a(j, l);
        for (i = j + 1 - 1; i < c.height(); ++i)
        {
          c(i, j) = c(i, j) + temp * a(i, l);
        }
      }
    }
  }
  return;
}

template class Blas<double, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
                    Computes(_C = _alpha * _A * T(_A) + _beta * _C),
                    platform::Parallelisation::NOT_PARALLEL>;

template class Blas<float, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
                    Computes(_C = _alpha * _A * T(_A) + _beta * _C),
                    platform::Parallelisation::NOT_PARALLEL>;

}  // namespace linalg
}  // namespace math
}  // namespace fetch