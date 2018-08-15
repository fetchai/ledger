#pragma once

#include "math/linalg/blas/base.hpp"
#include "math/linalg/matrix.hpp"
#include "math/linalg/prototype.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename S>
class Blas<S, Signature(L(_C) <= _alpha, L(_A), _beta, L(_C)),
           Computes(_C = _alpha * _A * T(_A) + _beta * _C), platform::Parallelisation::NOT_PARALLEL>
{
public:
  using type                 = S;
  using vector_register_type = typename Matrix<type>::vector_register_type;

  void operator()(type const &alpha, Matrix<type> const &a, type const &beta,
                  Matrix<type> &c) const;
};

}  // namespace linalg
}  // namespace math
}  // namespace fetch